/** $Id: load.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file load.c
	@addtogroup load_glm GLM file loader
	@ingroup core

	@note The function of GLM files has evolved from Version 1.  Now GLM files
	are used to synthesize models, whereas XML files are used to store models.

	@bug This loader uses a crude parser.  Someday it will be replaced by a more robust
	implementation that will be easier to fix, manage, improve, update, grow, etc.

	@par Comments
	All text from the first instance of "//" to the end of a line is treated as
	a comment.  The "//" must follow a whitespace to be considered a comment delimiter.
	When a "//" follows a non-white character, it is not recognized as a comment delimiter.
	@code
	// comments are preceded by two slashes, which must follow a white-space
	#set urlbase=http://www.nowhere.net/ // www.nowhere.net/ is not a comment
	@endcode

	@par Variables
	Any global variable or environment variable can be substituted into the incoming
	stream by using the \p ${varname} syntax.  First global variables are matched, and
	if none is found, then environment variables are matched.  If the global variable
	name is provided in the form \p ${module::varname} then the module global is
	substituted if it exists.

	@todo Command substitution using \p $(command).

	@todo Regular expressions using \p ${varname/from/to} and \p ${varname//from/to}.

	@par Macros
	The "#" character at the beginning of a line followed by a non-white character
	will cause the word that follows to be treated as a macro.  When a "#" is followed
	by a white-space it will cause the parser to consider the file to be an old-style
	GLM file and abandon the loading process.

	The following macros are recognized:
@code
#set <global>="<value>" // sets the existing global variable to the value

#define <global>="<value>" // defines a new global variable as the value

#setenv <variable>=<expression> // set the environment variable to the expression

#include "<file>" // includes the file in the load process

#if <expression> // includes the text if expression is non-zero otherwise includes alternate
... <text> ...
#else
... <alternate> ...
#endif

#ifdef <global> // includes the text if global is defined otherwise includes alternate
... <text> ...
#else
... <alternate> ...
#endif

#ifndef <global> // includes the text if global is undefined otherwise includes alternate
... <text> ...
#else
... <alternate> ...
#endif

#ifexist <file> // includes the text if file exists otherwise includes alternate
... <text> ...
#else
... <alternate> ...
#endif

#print <expression> // displays the expression on the output stream

#error <expression> // displays the expression on the error stream and fails the load

#warning <expression> // displays the warning on the warning stream

@endcode

	The following blocks are recognized at the root level:
@code
clock {
	timezone "ZZZTTTZZZ"; // sets the timezone to use for the simulation
	timestamp "YYYY-MM-DD HH:MM:SS ZZZ"; // sets the global clock starting time
}

global {
	<global> <expression>; // sets the global to the expression
}

module {
	<variable> <expression>; // sets the module variable to expression
	class <class> { // verifies existing static class
		<type> <property>[<unit>]; // defines /verifies a property in class
	}
}
	
class <class> { // creates a new runtime class

	// publish a GridLAB-D property
	<type> <property>[<unit>]; 

	// define an intrinsic function (i.e., create, init, presync, sync, postsync, etc.)
	intrinsic <name> ( <arglist> ) 
	{
		<C++ code>
	};

	// embed a C++ member variable or function
	public|protected|private <C++ member definition>;

	// declare a function in another class
	function <class-name>::<function-name>;

	// publish a C++ member function
	export <C++ member definition>;
}

object <class>[:<spec>] { // spec may be <id>, or <startid>..<endid>, or ..<count>
	<property> <expression>;
	[parent] object ...; // create a child object
	<object-property> object ...; // reference another object
}
@endcode

 @{

 **/

#ifdef HAVE_CONFIG_H
#include "config.h"
#else // not a build using automake
#define DLEXT ".dll"
#define REALTIME_LDFLAGS ""
#endif // HAVE_CONFIG_H
#ifndef REALTIME_LDFLAGS
#define REALTIME_LDFLAGS ""
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include "stream.h"
#include "http_client.h"

/* define this to use # for comment and % for macros (the way Version 1.x works) */
/* #define OLDSTYLE	*/

#ifdef OLDSTYLE
#define COMMENT "#"
#define MACRO "%"
#else
#define COMMENT "//"
#define MACRO "#"
#endif

#ifdef WIN32
#include <io.h>
#include <process.h>
#include <direct.h>
typedef struct _stat STAT;
#define FSTAT _fstat
#define tzset _tzset
#define snprintf _snprintf
#else
#include <unistd.h>
typedef struct stat STAT;
#define FSTAT fstat
#endif

#include "cmdarg.h"
#include "complex.h"
#include "object.h"
#include "load.h"
#include "output.h"
#include "random.h"
#include "convert.h"
#include "schedule.h"
#include "transform.h"
#include "instance.h"
#include "linkage.h"
#include "gui.h"

static unsigned int linenum=1;
static int include_fail = 0;
static char filename[1024];
static time_t modtime = 0;
static int last_good_depth = -1;
static int current_depth = -1;

static char start_ts[64];
static char stop_ts[64];

static char *format_object(OBJECT *obj)
{
	static char256 buffer;
	strcpy(buffer,"(unidentified)");
	if (obj->name==NULL)
		sprintf(buffer,global_object_format,obj->oclass->name,obj->id);
	else
		sprintf(buffer,"%s (%s:%d)",obj->name,obj->oclass->name, obj->id);
	return buffer;
}

static char *strip_right_white(char *b){
	size_t len, i;
	len = strlen(b) - 1;
	for(i = len; i >= 0; --i){
		if(b[i] == '\r' || b[i] == '\n' || b[i] == ' ' || b[i] == '\t'){
			b[i] = '\0';
		} else {
			break;
		}
	}
	return b;
}

/* inline source code support */
char *code_block = NULL;
char *global_block = NULL;
char *init_block = NULL;
int code_used = 0;

int inline_code_init(void)
{
	if ( code_block==NULL )
	{
		code_block = malloc(global_inline_block_size);
		if ( code_block==NULL ) {
			output_error("code_block malloc failed (inline_block_size=%d)", global_inline_block_size);
			/* TROUBLESHOOT
			   The memory allocation for the inline code block failed.
			   Try freeing up memory or reducing the size of the inline blocks (inline_block_size global variable).
			 */
			return 0;
		}
		memset(code_block,0,global_inline_block_size);
	}
	if ( global_block==NULL )
	{
		global_block = malloc(global_inline_block_size);
		if ( global_block==NULL ) {
			output_error("global_block malloc failed (inline_block_size=%d)", global_inline_block_size);
			/* TROUBLESHOOT
			   The memory allocation for the inline global code block failed.
			   Try freeing up memory or reducing the size of the inline blocks (inline_block_size global variable).
			 */
			return 0;
		}
		memset(global_block,0,global_inline_block_size);
	}
	if ( init_block==NULL )
	{
		init_block = malloc(global_inline_block_size);
		if ( init_block==NULL ) {
			output_error("init_block malloc failed (inline_block_size=%d)", global_inline_block_size);
			/* TROUBLESHOOT
			   The memory allocation for the inline init block failed.
			   Try freeing up memory or reducing the size of the inline blocks (inline_block_size global variable).
			 */
			return 0;
		}
		memset(init_block,0,global_inline_block_size);
	}
	return 1;
}

void inline_code_term(void)
{
	if ( code_block!=NULL )
	{
		free(code_block);
		code_block = NULL;
	}
	if ( global_block!=NULL )
	{
		free(global_block);
		global_block = NULL;
	}
	if ( init_block!=NULL )
	{
		free(init_block);
		init_block = NULL;
	}
	return;
}


/* used to track which functions are included in runtime classes */
#define FN_CREATE		0x0001
#define FN_INIT			0x0002
#define FN_NOTIFY		0x0004
#define FN_RECALC		0x0008
#define FN_PRESYNC		0x0010
#define FN_SYNC			0x0020
#define FN_POSTSYNC		0x0040
#define FN_PLC			0x0080
#define FN_ISA			0x0100
#define FN_COMMIT		0x0200
#define FN_PRECOMMIT	0x0400
#define FN_FINALIZE		0x0800
#define FN_EXPORT		0x1000

/* used for tracking #include directives in files */
#define BUFFERSIZE (65536*1000)
typedef struct s_include_list {
	char file[256];
	struct s_include_list *next;
} INCLUDELIST;

INCLUDELIST *include_list = NULL;
INCLUDELIST *header_list = NULL;

//UNR_STATIC *static_list = NULL;

static char *forward_slashes(char *a)
{
	static char buffer[1024];
	char *b=buffer;
	while (*a!='\0' && b<buffer+sizeof(buffer))
	{
		if (*a=='\\')
			*b = '/';
		else
			*b = *a;
		a++;
		b++;
	}
	*b='\0';
	return buffer;
}

/* extract parts of a filename */
static void filename_parts(char *fullname, char *path, char *name, char *ext)
{
	/* fix delimiters (result is a static copy) */
	char *file = forward_slashes(fullname);

	/* find the last delimiter */
	char *s = strrchr(file,'/');

	/* find the last dot */
	char *e = strrchr(file,'.');

	/* clear results */
	path[0] = name[0] = ext[0] = '\0';
	
	/* if both found but dot is before delimiter */
	if (e && s && e<s) 
		
		/* there is not extension */
		e = NULL;
	
	/* copy extension (if any) and terminate filename at dot */
	if (e)
	{
		strcpy(ext,e+1);
		*e = '\0';
	}

	/* if path is given */
	if (s)
	{
		/* copy name and terminate path */
		strcpy(name,s+1);
		*s = '\0';

		/* copy path */
		strcpy(path,file);
	}

	/* otherwise copy everything */
	else
		strcpy(name,file);
}


static int append_init(char* format,...)
{
	static char code[1024];
	va_list ptr;
	va_start(ptr,format);
	vsprintf(code,format,ptr);
	va_end(ptr);

	if (strlen(init_block)+strlen(code)>global_inline_block_size)
	{
		output_fatal("insufficient buffer space to compile init code (inline_block_size=%d)", global_inline_block_size);
		/*	TROUBLESHOOT
			The loader creates a buffer in which it can temporarily hold source
			initialization code from your GLM file.  This error occurs when the buffer space
			has been exhausted.  There are only two ways to fix this problem,
			1) make the code smaller (which can be difficult to do), or 
			2) increase the buffer space (which requires a rebuild).
		*/
		return 0;
	}
	strcat(init_block,code);
	return ++code_used;
}
static int append_code(char* format,...)
{
	static char code[65536];
	va_list ptr;
	va_start(ptr,format);
	vsprintf(code,format,ptr);
	va_end(ptr);

	if (strlen(code_block)+strlen(code)>global_inline_block_size)
	{
		output_fatal("insufficient buffer space to compile init code (inline_block_size=%d)", global_inline_block_size);
		/*	TROUBLESHOOT
			The loader creates a buffer in which it can temporarily hold source
			runtime code from your GLM file.  This error occurs when the buffer space
			has been exhausted.  There are only two ways to fix this problem,
			1) make the code smaller (which can be difficult to do), or 
			2) increase the buffer space (which requires a rebuild).
		*/
		return 0;
	}
	strcat(code_block,code);
	return ++code_used;
}
static int append_global(char* format,...)
{
	static char code[1024];
	va_list ptr;
	va_start(ptr,format);
	vsprintf(code,format,ptr);
	va_end(ptr);

	if (strlen(global_block)+strlen(code)>global_inline_block_size)
	{
		output_fatal("insufficient buffer space to compile init code (inline_block_size=%d)", global_inline_block_size);
		/*	TROUBLESHOOT
			The loader creates a buffer in which it can temporarily hold source
			global code from your GLM file.  This error occurs when the buffer space
			has been exhausted.  There are only two ways to fix this problem,
			1) make the code smaller (which can be difficult to do), or 
			2) increase the buffer space (which requires a rebuild).
		*/
		return 0;
	}
	strcat(global_block,code);
	return ++code_used;
}
static void mark_linex(char *filename, int linenum)
{
	char buffer[64];
	if (global_getvar("noglmrefs",buffer, 63)==NULL)
		append_code("#line %d \"%s\"\n", linenum, forward_slashes(filename));
}
static void mark_line()
{
	mark_linex(filename,linenum);
}
static STATUS exec(char *format,...)
{
	char cmd[1024];
	va_list ptr;
	va_start(ptr,format);
	vsprintf(cmd,format,ptr);
	va_end(ptr);
	output_debug("Running '%s' in '%s'", cmd, getcwd(NULL,0));
	return system(cmd)==0?SUCCESS:FAILED;
}

static STATUS debugger(char *target)
{
	int result;
	output_debug("Starting debugger");
#ifdef _MSC_VER
#define getpid _getpid
	result = exec("start %s gdb --quiet %s --pid=%d",global_gdb_window?"":"/b",target,global_process_id)>=0?SUCCESS:FAILED;
	system("pause");
#else
	output_debug("Use 'dll-symbols %s' to load symbols",target);
	result = exec("gdb --quiet %s --pid=%d &",target,global_process_id)>=0?SUCCESS:FAILED;
#endif
	return result;
}

static char *setup_class(CLASS *oclass)
{
	static char buffer[65536] = "";
	int len = 0;
	PROPERTY *prop;
	len += sprintf(buffer+len,"\tOBJECT obj; obj.oclass = oclass; %s *t = (%s*)((&obj)+1);\n",oclass->name,oclass->name);
	//len += sprintf(buffer+len,"\tif (callback->define_map(oclass,\n");
	len += sprintf(buffer+len,"\toclass->size = sizeof(%s);\n", oclass->name);
	for (prop=oclass->pmap; prop!=NULL; prop=prop->next)
	{
		len += sprintf(buffer+len,"\t(*(callback->properties.get_property))(&obj,\"%s\",NULL)->addr = (PROPERTYADDR)((char*)&(t->%s) - (char*)t);\n",prop->name,prop->name);
#ifdef NEVER
		if (prop->unit==NULL)
			len += sprintf(buffer+len,"\t\tPT_%s,\"%s\",(char*)&(t->%s)-(char*)t,\n",
				class_get_property_typename(prop->ptype),prop->name,prop->name);
		else
			len += sprintf(buffer+len,"\t\tPT_%s,\"%s[%s]\",(char*)&(t->%s)-char(*)t,\n",
				class_get_property_typename(prop->ptype),prop->name,prop->unit->name,prop->name);
		if (prop->keywords)
		{
			KEYWORD *key;
			for (key=prop->keywords; key!=NULL; key=key->next)
				len += sprintf(buffer+len, "\t\t\tPT_KEYWORD, \"%s\", %d,\n", key->name, key->value);
		}
#endif
	}
	//len += sprintf(buffer+len,"\t\tNULL)<1) throw(\"unable to publish properties in class %s\");\n", oclass->name);
	len += sprintf(buffer+len,"%s\n",init_block);
	return buffer;
}

static int outlinenum = 0;
static char *outfilename = NULL;
static int write_file(FILE *fp, char *data, ...)
{
	char buffer[65536];
	char var_buf[64];
	char *c, *d=buffer;
	int len=0;
	int diff = 0;
	char *b;
	va_list ptr;
	va_start(ptr,data);
	vsprintf(buffer,data,ptr);
	va_end(ptr);
	while ((c=strstr(d,"/*RESETLINE*/\n"))!=NULL)
	{
		for (b=d; b<c; b++)
		{
			if (*b=='\n')
				outlinenum++;
			fputc(*b,fp);
			diff++;
			len++;
		}
		d =  c + strlen("/*RESETLINE*/\n");
		if (global_getvar("noglmrefs",var_buf,63)==NULL)
			len += fprintf(fp,"#line %d \"%s\"\n", ++outlinenum+1,forward_slashes(outfilename));
	}
	for (b=d; *b!='\0'; b++)
	{
		if (*b=='\n')
			outlinenum++;
		fputc(*b,fp);
		len++;
	}
	return len;
}
static void reset_line(FILE *fp, char *file)
{
	char buffer[64];
	if (global_getvar("noglmrefs", buffer, 63)==NULL)
		write_file(fp,"#line %s \"%s\"\n", outlinenum,forward_slashes(file));
}

// Recursively make directories if they don't exist
static int mkdirs(char *path)
{
	int rc;
	//struct stat st;

#ifdef WIN32
#	define PATHSEP '\\'
#	define mkdir(P,M) _mkdir((P)) // windows does not use mode info
#	define access _access
#else
#	define PATHSEP '/'
#endif

	if (!path) {
		errno = EINVAL;
		return -1;
	}
	if ((rc = access(path, F_OK)) && errno == ENOENT) {
		// path doesn't exist
		char *pos, *end, *tmp;
		output_verbose("creating directory '%s'", path);
		if (!(tmp = (char *) malloc(strlen(path) + 1))) {
			errno = ENOMEM;
			output_fatal("mkdirs() failed: '%s'", strerror(errno));
			return -1;
		}
		strcpy(tmp, path);
		end = tmp + strlen(tmp);
		// strip off directories until one is found that exists
		while ((pos = strrchr(tmp, PATHSEP))) {
			*pos = '\0';
			if (!(*tmp) || !(rc = access(tmp, F_OK)))
				break;
			if (errno != ENOENT) {
				output_error("cannot access directory '%s': %s", tmp, strerror(errno));
				free(tmp);
				tmp = NULL;
				return -1;
			}
		}
		// add back components creating them as we go
		for (pos = tmp + strlen(tmp); pos < end; pos = tmp + strlen(tmp)) {
			if (*pos == '\0') {
				*pos = PATHSEP;
				if ((rc = mkdir(tmp, 0775)) && errno != EEXIST) {
					output_error("cannot create directory '%s': %s", tmp, strerror(errno));
					free(tmp);
					tmp = NULL;
					return -1;
				}
			}
		}
		free(tmp);
		tmp = NULL;
		return 0;
	} else if (rc)
		output_error("cannot access directory '%s': %s", path, strerror(errno));
	return rc;
}

static STATUS compile_code(CLASS *oclass, int64 functions)
{
	char include_file_str[1024];
	char buffer[256];
	bool use_msvc = (global_getvar("use_msvc",buffer,255)!=NULL);

	include_file_str[0] = '\0';

	/* check global_include */
	if (strlen(global_include)==0)
	{
		if (getenv("GRIDLABD"))
		{
			strncpy(global_include,getenv("GRIDLABD"),sizeof(global_include));
			output_verbose("global_include is not set, assuming value of GRIDLABD variable '%s'", global_include);
		}
		else
		{
			output_error("'include' variable is not set and neither is GRIDLABD environment, compiler cannot proceed without a way to find rt/gridlabd.h");
			/* TROUBLESHOOT
				The runtime class compiler needs to find the file rt/gridlabd.h and uses either the <i>include</i> global variable or the <b>gridlabd</b> 
				environment variable to find it.  Check the definition of the <b>gridlabd</b> environment variable or use the 
				<code>#define include=<i>path</i></code> to specify the path to the <code>rt/gridlabd.h</code>.
			 */
			return FAILED;
		}
	}

	if (code_used>0)
	{
		MODULE *mod;

		FILE *fp;
		STAT stat;
		int outdated = true;
		char cfile[1024];
		char ofile[1024];
		char afile[1024];
		char file[1024];
		char tmp[1024];
		char tbuf[64];
		size_t ifs_off = 0;
		INCLUDELIST *lptr = 0;

		/* build class implementation files */
		strncpy(tmp, global_tmp, sizeof(tmp));
		if (mkdirs(tmp)) {
			errno = 0;
			return FAILED;
		}
		if (strlen(tmp)>0 && tmp[strlen(tmp)-1]!='/' && tmp[strlen(tmp)-1]!='\\')
			strcat(tmp,"/");
		sprintf(cfile,"%s%s.cpp", (use_msvc||global_gdb||global_gdb_window)?"":tmp,oclass->name);
		sprintf(ofile,"%s%s.%s", (use_msvc||global_gdb||global_gdb_window)?"":tmp,oclass->name, use_msvc?"obj":"o");
		sprintf(file,"%s%s", (use_msvc||global_gdb||global_gdb_window)?"":tmp, oclass->name);
		sprintf(afile, "%s" DLEXT , oclass->name);

		/* peek at library file */
		fp = fopen(afile,"r");
		if (fp!=NULL && FSTAT(fileno(fp),&stat)==0)
		{
			if (global_debug_mode || use_msvc || global_gdb || global_gdb_window )
			{
				output_verbose("%s is being used for debugging", afile);
			}
			else if (global_force_compile)
				output_verbose("%s recompile is forced", afile);
			else if (modtime<stat.st_mtime)
			{
				output_verbose("%s is up to date", afile);
				outdated = false;
			}
			else
				output_verbose("%s is outdated", afile);
			fclose(fp);
		}
		
		if (outdated)
		{
			/* write source file */
			fp = fopen(cfile,"w");

			output_verbose("writing inline code to '%s'", cfile);
			if (fp==NULL)
			{
				output_fatal("unable to open '%s' for writing", cfile);
				/*	TROUBLESHOOT
					The internal compiler cannot write a temporary C or C++ that
					it needs to build your model.  The message indicates where
					the file is.  To remedy the problem you must make sure that
					the system allows full access to the file.
				 */
				return FAILED;
			}
			outfilename = cfile;
			ifs_off = 0;
			for(lptr = header_list; lptr != 0; lptr = lptr->next){
				sprintf(include_file_str+ifs_off, "#include \"%s\"\n;", lptr->file);
				ifs_off+=strlen(lptr->file)+13;
			}
			if (write_file(fp,"/* automatically generated from GridLAB-D */\n\n"
					"int major=0, minor=0;\n\n"
					"%s\n\n"
					"#include \"rt/gridlabd.h\"\n\n"
					"%s"
					"CALLBACKS *callback = NULL;\n"
					"static CLASS *myclass = NULL;\n"
					"static int setup_class(CLASS *);\n\n",
					include_file_str,
					global_getvar("use_msvc",tbuf,63)!=NULL
					?
						"int __declspec(dllexport) dllinit() { return 0;};\n"
						"int __declspec(dllexport) dllkill() { return 0;};\n"
					:
						"extern \"C\" int dllinit() __attribute__((constructor));\n"
						"extern \"C\" int dllinit() { return 0;}\n"
						"extern \"C\"  int dllkill() __attribute__((destructor));\n"
						"extern \"C\" int dllkill() { return 0;};\n\n"
					)<0
				|| write_file(fp,"extern \"C\" CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])\n"
					"{\n"
					"\tcallback=fntable;\n"
					"\tmyclass=(CLASS*)((*(callback->class_getname))(\"%s\"));\n"
					"\tif (!myclass) return NULL;\n"
					"\tif (!setup_class(myclass)) return NULL;\n"
					"\treturn myclass;"
					"}\n",oclass->name)<0
				|| write_file(fp,"%s",code_block)<0 
				|| write_file(fp,"%s",global_block)<0
				|| write_file(fp,"static int setup_class(CLASS *oclass)\n"
					"{\t\n%s\treturn 1;\n}\n",setup_class(oclass))<0 
				)
			{
				output_fatal("unable to write to '%s'", cfile);
				/*	TROUBLESHOOT
					The internal compiler cannot write a temporary C or C++ that
					it needs to build your model.  The message indicates where
					the file is.  To remedy the problem you must make sure that
					the system allows full access to the file.
				 */
				return FAILED;
			}
			fclose(fp);
			outfilename=NULL;

			/* compile object file */
			output_verbose("compiling inline code from '%s'", cfile);
			output_debug("PATH=%s", getenv("PATH"));
			output_debug("INCLUDE=%s", getenv("INCLUDE"));
			output_debug("LIB=%s", getenv("LIB"));
			if (!use_msvc)
			{
#ifdef WIN32
#define EXTRA_CXXFLAGS ""
#else
#define EXTRA_CXXFLAGS "-fPIC"
#endif

				char execstr[1024];
				char ldstr[1024];
				char exportsyms[64] = REALTIME_LDFLAGS;
				char mopt[8]="";
				char *libs = "-lstdc++";
#ifdef WIN32
				snprintf(mopt,sizeof(mopt),"-m%d",sizeof(void*)*8);
				libs = "";
#endif

				sprintf(execstr, "%s %s -w %s %s -I \"%s\" -c \"%s\" -o \"%s\"", getenv("CXX")?getenv("CXX"):"g++", 
					getenv("CXXFLAGS")?getenv("CXXFLAGS"):EXTRA_CXXFLAGS, mopt, global_debug_output?"-g -O0":"", global_include, cfile, ofile);
				output_verbose("compile string: \"%s\"", execstr);
				//if (exec("g++ %s -I\"%s\" -c \"%s\" -o \"%s\"", global_debug_output?"-g -O0":"", global_include, cfile, ofile)==FAILED)
				if(exec(execstr)==FAILED)
					return FAILED;
				if (!(global_gdb||global_gdb_window))
					unlink(cfile);


				/* link new runtime module */
				output_verbose("linking inline code from '%s'", ofile);
				sprintf(ldstr, "%s %s %s %s %s -shared -Wl,\"%s\" -o \"%s\" %s", getenv("CXX")?getenv("CXX"):"g++" , mopt, getenv("LDFLAGS")?getenv("LDFLAGS"):EXTRA_CXXFLAGS, global_debug_output?"-g -O0":"", exportsyms, ofile,afile,libs);
				output_verbose("linking string: \"%s\"", ldstr);
				//if (exec("%s %s %s %s -shared -Wl,\"%s\" -o \"%s\" -lstdc++", getenv("CXX")?getenv("CXX"):"g++" , getenv("LDFLAGS")?getenv("LDFLAGS"):EXTRA_CXXFLAGS, global_debug_output?"-g -O0":"", exportsyms, ofile,afile)==FAILED)
				if(exec(ldstr) == FAILED)
					return FAILED;

				if (global_getvar("control_textrel_shlib_t",tbuf,63)!=NULL)
				{
					/* SE linux need the new module marked as relocatable (textrel_shlib_t) */
					exec("chcon -t textrel_shlib_t '%s'", afile);
				}

				unlink(ofile);
			}
			else
			{
				char exports[1024] = "/EXPORT:init ";

				sprintf(exports+strlen(exports),"/EXPORT:create_%s ",oclass->name); /* create is required */
				if (functions&FN_INIT) sprintf(exports+strlen(exports),"/EXPORT:init_%s ",oclass->name);
				if (functions&FN_PRECOMMIT) sprintf(exports+strlen(exports),"/EXPORT:precommit_%s ",oclass->name);
				if (functions&FN_PRESYNC || functions&FN_SYNC || functions&FN_POSTSYNC) sprintf(exports+strlen(exports),"/EXPORT:sync_%s ",oclass->name);
				if (functions&FN_ISA) sprintf(exports+strlen(exports),"/EXPORT:isa_%s ",oclass->name);
				if (functions&FN_NOTIFY) sprintf(exports+strlen(exports),"/EXPORT:notify_%s ",oclass->name);
				if (functions&FN_PLC) sprintf(exports+strlen(exports),"/EXPORT:plc_%s ",oclass->name);
				if (functions&FN_RECALC) sprintf(exports+strlen(exports),"/EXPORT:recalc_%s ",oclass->name);
				if (functions&FN_COMMIT) sprintf(exports+strlen(exports),"/EXPORT:commit_%s ",oclass->name);
				if (functions&FN_FINALIZE) sprintf(exports+strlen(exports),"/EXPORT:finalize_%s ",oclass->name);

				if (exec("cl /Od /DWIN32 /D_DEBUG /D_WINDOWS /D_USRDLL /D_CRT_SECURE_NO_DEPRECATE /D_WINDLL /D_MBCS /Gm /EHsc /RTC1 "
#if __WORDSIZE__==64
					"/Wp64 "
#endif
					"/MDd /nologo /W3 /Zi /TP /wd4996 /errorReport:none /c %s  %s%s%s /Fo%s"
					"", cfile, strlen(global_include)>0?"/I \"":"", global_include, strlen(global_include)>0?"\"":"", ofile)==FAILED)
				{
					output_error("MSVC compile failed for '%s'", cfile);
					return FAILED;
				}

				if (exec("link %s /OUT:%s /NOLOGO /DLL /MANIFEST /MANIFESTFILE:%s.manifest "
					"/SUBSYSTEM:WINDOWS "
#ifdef _DEBUG
					"/DEBUG "
#endif
#if __WORDSIZE__==64
					"/MACHINE:X64 "
#else
					"/MACHINE:X86 "
#endif
					"/ERRORREPORT:NONE "
					"%s "
					"kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib "
					"uuid.lib odbc32.lib odbccp32.lib", file,afile,oclass->name,exports)==FAILED)
				{
					output_error("MSVC link failed for '%s'", cfile);
					return FAILED;
				}
			}

		}

		/* load runtime module */
		output_verbose("loading dynamic link library %s...", afile);
		mod = module_load(oclass->name,0,NULL);
		if (mod==NULL)
		{
			output_error("unable to load inline code");
			return FAILED;
		}
		oclass->module = mod;

		/* start debugger if requested */
		if (global_gdb || global_gdb_window)
		{
			if (global_debug_mode)
				output_debug("using gdb requires GLD debugger be disabled");
			global_debug_output = 1;
			output_verbose("attaching debugger to process id %d", getpid());
			if (debugger(afile)==FAILED)
			{
				output_error("debugger load failed: %s", errno?strerror(errno):"(no details)");
				return FAILED;
			}
		}

		/* provide info so external debugger can be attached */
		else
			output_debug("class %s running in process %d", oclass->name, getpid());

		oclass->has_runtime = true;
		strcpy(oclass->runtime,afile);
	}
	else
		oclass->has_runtime = false;
	return SUCCESS;
}


static OBJECT **object_index = NULL;
static unsigned char *object_linked = NULL;
static unsigned int object_index_size = 65536;
/*static*/ STATUS load_set_index(OBJECT *obj, OBJECTNUM id)
{
	if (object_index==NULL)
	{
		object_index = malloc(sizeof(OBJECT*)*object_index_size);
		memset(object_index,0,sizeof(OBJECT*)*object_index_size);
		object_linked = malloc(sizeof(unsigned char)*object_index_size);
		memset(object_linked,0,sizeof(unsigned char)*object_index_size);
	}
	if (id>=object_index_size) /* index needs to grow */
	{
		int new_size = (id/object_index_size+1)*object_index_size;
		object_index = realloc(object_index,sizeof(OBJECT*)*new_size);
//		memset(object_index+object_index_size*sizeof(OBJECT *),0,sizeof(OBJECT*)*(new_size-object_index_size));
		object_linked = realloc(object_linked,sizeof(unsigned char)*new_size);
//		memset(object_linked+object_index_size*sizeof(unsigned char),0,sizeof(unsigned char)*(new_size-object_index_size));
		object_index_size = new_size;
	}
	if (object_index==NULL) { errno = ENOMEM; return FAILED;}
	/* collision check here */
	object_index[id] = obj;
	object_linked[id] = 0;
	return SUCCESS;
}
/*static*/ OBJECT *load_get_index(OBJECTNUM id)
{
	if (object_index==NULL || id<0 || id>=object_index_size)
		return NULL;
	object_linked[id]++;
	return object_index[id];
}
static OBJECT *get_next_unlinked(CLASS *oclass)
{
	unsigned int id;
	if (object_index==NULL)
		return NULL;
	for (id=0; id<object_index_size; id++)
	{
		if (object_linked[id]==0 && object_index[id]!=NULL && object_index[id]->oclass==oclass)
		{
			object_linked[id]++;
			return object_index[id];
		}
	}
	return NULL;
}
static void free_index(void)
{
	if (object_index!=NULL)
		free(object_index);
	object_index=NULL;
	object_index_size = 65536;
}

static UNRESOLVED *first_unresolved = NULL;
/*static*/ UNRESOLVED *add_unresolved(OBJECT *by, PROPERTYTYPE ptype, void *ref, CLASS *oclass, char *id, char *file, unsigned int line, int flags)
{
	UNRESOLVED *item;
	if ( strlen(id)>=sizeof(item->id))
	{
		output_error("add_unresolved(...): id '%s' is too long to resolve", id);
		return NULL;
	}
	item = malloc(sizeof(UNRESOLVED));
	if (item==NULL) { errno = ENOMEM; return NULL; }
	item->by = by;
	item->ptype = ptype;
	item->ref = ref;
	item->oclass = oclass;
	strncpy(item->id,id,sizeof(item->id));
	if (first_unresolved!=NULL && strcmp(first_unresolved->file,file)==0)
	{
		item->file = first_unresolved->file; // means keep using the same file
		first_unresolved->file = NULL;
	}
	else
	{
		item->file = (char*)malloc(strlen(file)+1);
		strcpy(item->file,file);
	}
	item->line = line;
	item->next = first_unresolved;
	item->flags = flags;
	first_unresolved = item;
	return item;
}
static int resolve_object(UNRESOLVED *item, char *filename)
{
	OBJECT *obj;
	char classname[65];
	char propname[65];
	OBJECTNUM id = 0;
	char op[2];
	char star;

	if(0 == strcmp(item->id, "root"))
		obj = NULL;
	else if (sscanf(item->id,"%64[^.].%64[^:]:",classname,propname)==2)
	{
		char *value = strchr(item->id,':');
		FINDLIST *match;
		if (value++==NULL)
		{
			output_error_raw("%s(%d): %s reference to %s is missing match value", filename, item->line,
				format_object(item->by), item->id);
			return FAILED;
		}
		match = find_objects(FL_NEW,FT_CLASS,SAME,classname,AND,FT_PROPERTY,propname,SAME,value,FT_END);
		if (match==NULL || match->hit_count==0)
		{
			output_error_raw("%s(%d): %s reference to %s does not match any existing objects", filename, item->line,
				format_object(item->by), item->id);
			return FAILED;
		}
		else if (match->hit_count>1)
		{
			output_error_raw("%s(%d): %s reference to %s matches more than one object", filename, item->line,
				format_object(item->by), item->id);
			return FAILED;
		}
		obj=find_first(match);
	}
	else if (sscanf(item->id,"%[^:]:id%[+-]%d",classname,op,&id)==3)
	{
		CLASS *oclass = class_get_class_from_classname(classname);
		obj = object_find_by_id(item->by->id + (op[0]=='+'?+1:-1)*id);
		if (obj==NULL)
		{
			output_error_raw("%s(%d): unable resolve reference from %s to %s", filename, item->line,
				format_object(item->by), item->id);
			return FAILED;
		}
	}
	else if (sscanf(item->id,global_object_scan,classname,&id)==2)
	{
		obj = load_get_index(id);
		if (obj==NULL)
		{
			output_error_raw("%s(%d): unable resolve reference from %s to %s", filename, item->line,
				format_object(item->by), item->id);
			return FAILED;
		}
		if ((strcmp(obj->oclass->name,classname)!=0) && (strcmp("id", classname) != 0))
		{ /* "id:###" is our wildcard.  some converters use it for dangerous simplicity. -mh */
			output_error_raw("%s(%d): class of reference from %s to %s mismatched", filename, item->line,
				format_object(item->by), item->id);
			return FAILED;
		}
	}
	else if (sscanf(item->id,"%[^:]:%c",classname,&star)==2 && star=='*')
	{
		CLASS *oclass = class_get_class_from_classname(classname);
		obj = get_next_unlinked(oclass);
		if (obj==NULL)
		{
			output_error_raw("%s(%d): unable resolve reference from %s to %s", filename, item->line,
				format_object(item->by), item->id);
			return FAILED;
		}
	}
	else if ((obj=object_find_name(item->id))!=NULL)
	{
		/* found it already*/
	}
	else
	{
		output_error_raw("%s(%d): '%s' not found", filename, item->line, item->id);
		return FAILED;
	}
	*(OBJECT**)(item->ref) = obj;
	if ((item->flags&UR_RANKS)==UR_RANKS)
		object_set_rank(obj,item->by->rank);
	return SUCCESS;
}
static int resolve_double(UNRESOLVED *item, char *context)
{
	FULLNAME oname;
	PROPERTYNAME pname;
	char *filename = (item->file ? item->file : context);

	if (sscanf(item->id,"%64[^.].%64s",oname,pname)==2)
	{
		OBJECT *obj = NULL;
		PROPERTY *prop = NULL;
		double **ref = NULL;
		TRANSFORM *xform = NULL;

		/* get and check the object */
		obj = object_find_name(oname);
		if (obj==NULL)
		{
			output_error_raw("%s(%d): object '%s' not found", filename, item->line, oname);
			return FAILED;
		}

		/* get and check the property */
		prop = object_get_property(obj,pname,NULL);
		if (prop==NULL)
		{
			output_error_raw("%s(%d): property '%s' not found", filename, item->line, pname);
			return FAILED;
		}

		/* get transform reference */
		if ((item->flags&UR_TRANSFORM)==UR_TRANSFORM)
		{
			/* find transform that uses this target */
			while ((xform=transform_getnext(xform))!=NULL)
			{
				/* the reference is to the schedule's source */
				if (xform==item->ref)
				{
					ref = &(xform->source);
					break;
				}
			}
		}

		/* get the direct reference */
		else
			ref = (double**)(item->ref);
		
		/* extract the reference to the object property */
		switch (prop->ptype) {
		case PT_double:
			*ref = object_get_double(obj,prop);
			if (xform) xform->source_type = XS_DOUBLE;
			break;
		case PT_complex:
			*ref = &(((complex*)object_get_addr(obj,prop->name))->r);
			if (xform) xform->source_type = XS_COMPLEX;
			break;
		case PT_loadshape:
			*ref = &(((loadshape*)object_get_addr(obj,prop->name))->load);
			if (xform) xform->source_type = XS_LOADSHAPE;
			break;
		case PT_enduse:
			*ref = &(((enduse*)object_get_addr(obj,prop->name))->total.r);
			if (xform) xform->source_type = XS_ENDUSE;
			break;
		default:
			output_error_raw("%s(%d): reference '%s' type is not supported", filename, item->line, item->id);
			return FAILED;
		}

		output_debug("reference '%s' resolved ok", item->id);

		return SUCCESS;
	}
	return FAILED;
}

static int resolve_list(UNRESOLVED *item)
{
	UNRESOLVED *next;
	char *filename = NULL;
	while (item!=NULL)
	{	
		// context file name changes
		if (item->file!=NULL)
		{
			// free last context file name
			if (filename!=NULL){
				free(filename); // last one - not used again
				filename = NULL;
			}

			// get next context file name
			filename = item->file;
		}

		// handle different reference types
		switch (item->ptype) {
		case PT_object:
			if (resolve_object(item, filename)==FAILED)
				return FAILED;
			break;
		case PT_double:
		case PT_complex:
		case PT_loadshape:
		case PT_enduse:
			if (resolve_double(item, filename)==FAILED)
				return FAILED;
			break;
		default:
			output_error_raw("%s(%d): unresolved reference to property '%s' uses unsupported type (ptype=%d)", filename, item->line, item->id, item->ptype);
			break;
		}
		next = item->next;
		free(item);
		item=next;
	}
	return SUCCESS;
}
/*static*/ int load_resolve_all()
{
	return resolve_list(first_unresolved);
}

#define PARSER char *_p
#define START int _mm=0, _m=0, _n=0, _l=linenum;
#define ACCEPT { _n+=_m; _p+=_m; _m=0; }
#define HERE (_p+_m)
#define OR {_m=0;}
#define REJECT { linenum=_l; return 0; }
//#define WHITE (_m+=white(HERE))
#define WHITE (TERM(white(HERE)))
#define LITERAL(X) (_mm=literal(HERE,(X)),_m+=_mm,_mm>0)
#define TERM(X) (_mm=(X),_m+=_mm,_mm>0)
#define COPY(X) {size--; (X)[_n++]=*_p++;}
#define DONE return _n;
#define BEGIN_REPEAT {char *__p=_p; int __mm=_mm, __m=_m, __n=_n, __l=_l; int __ln=linenum;
#define REPEAT _p=__p;_m=__m; _mm=__mm; _n=__n; _l=__l; linenum=__ln;
#define END_REPEAT }

static void syntax_error(char *p)
{
	char context[16], *nl;
	strncpy(context,p,15);
	nl = strchr(context,'\n');
	if (nl!=NULL) *nl='\0'; else context[15]='\0';
	if (strlen(context)>0)
		output_error_raw("%s(%d): syntax error at '%s...'", filename, linenum, context);
	else
		output_error_raw("%s(%d): syntax error", filename, linenum);
}

static int white(PARSER)
{
	int len = 0;
	for(len = 0; *_p != '\0' && isspace((unsigned char)(*_p)); ++_p){
		if (*_p == '\n')
			++linenum;
		++len;
	}
	return len;
}


static int comment(PARSER)
{
	int _n = white(_p);
	if (_p[_n]=='#')
	{
		while (_p[_n]!='\n')
			_n++;
		linenum++;
	}
	return _n;
}

static int pattern(PARSER, char *pattern, char *result, int size)
{
	char format[64];
	START;
	sprintf(format,"%%%s",pattern);
	if (sscanf(_p,format,result)==1)
		_n = (int)strlen(result);
	DONE;
}

static int scan(PARSER, char *format, char *result, int size)
{
	START;
	if (sscanf(_p,format,result)==1)
		_n = (int)strlen(result);
	DONE;
}

static int literal(PARSER, char *text)
{
	if (strncmp(_p,text,strlen(text))==0)
		return (int)strlen(text);
	return 0;
}

static int dashed_name(PARSER, char *result, int size)
{	/* basic name */
	START;
	/* names cannot start with a digit */
	if (isdigit(*_p)) return 0;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p=='_' || *_p=='-') COPY(result);
	result[_n]='\0';
	DONE;
}

static int name(PARSER, char *result, int size)
{	/* basic name */
	START;
	/* names cannot start with a digit */
	if (isdigit(*_p)) return 0;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p=='_') COPY(result);
	result[_n]='\0';
	DONE;
}
static int namelist(PARSER, char *result, int size)
{	/* basic list of names */
	START;
	/* names cannot start with a digit */
	if (isdigit(*_p)) return 0;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p==',' || *_p=='@' || *_p==' ' || *_p=='_') COPY(result);
	result[_n]='\0';
	DONE;
}
static int variable_list(PARSER, char *result, int size)
{	/* basic list of variable names */
	START;
	/* names cannot start with a digit */
	if (isdigit(*_p)) return 0;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p==',' || *_p==' ' || *_p=='.' || *_p=='_') COPY(result);
	result[_n]='\0';
	DONE;
}

static int unitspec(PARSER, UNIT **unit)
{
	char result[1024];
	size_t size = sizeof(result);
	START;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p=='$' || *_p=='%' || *_p=='*' || *_p=='/' || *_p=='^') COPY(result);
	result[_n]='\0';
	TRY {
		if ((*unit=unit_find(result))==NULL){
			linenum=_l;
			_n = 0;
		} else {
			_n = (int)strlen(result);
		}
	}
	CATCH (char *msg) {
		linenum=_l;
		_n = 0;
	}
	ENDCATCH
	DONE;
}

static int unitsuffix(PARSER, UNIT **unit)
{
	START;
	if (LITERAL("["))
	{
		if (!TERM(unitspec(HERE,unit)))
		{
			output_error_raw("%s(%d): missing valid unit after [", filename, linenum);
			REJECT;
		}
		if (!LITERAL("]"))
		{
			output_error_raw("%s(%d): missing ] after unit '%s'", filename, linenum,(*unit)->name);
		}
		ACCEPT;
		DONE;
	}
	REJECT;
	DONE;
}

static int nameunit(PARSER,char *result,int size,UNIT **unit)
{
	START;
	if (TERM(name(HERE,result,size)) && TERM(unitsuffix(HERE,unit))) ACCEPT; DONE;
	REJECT;
}

static int dotted_name(PARSER, char *result, int size)
{	/* basic name */
	START;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p=='_' || *_p=='.') COPY(result);
	result[_n]='\0';
	DONE;
}

static int hostname(PARSER, char *result, int size)
{	/* full path name */
	START;
	while (size>1 && isalpha(*_p) || isdigit(*_p) || *_p=='_' || *_p=='.' || *_p=='-' || *_p==':' ) COPY(result);
	result[_n]='\0';
	DONE;
}

static int delim_value(PARSER, char *result, int size, char *delims)
{
	/* everything to any of delims */
	int quote=0;
	char *start=_p;
	START;
	if (*_p=='"')
	{
		quote=1;
		*_p++;
		size--;
	}
	while (size>1 && *_p!='\0' && ((quote&&*_p!='"') || strchr(delims,*_p)==NULL) && *_p!='\n') 
	{
		if ( _p[0]=='\\' && _p[1]!='\0' ) _p++; 
		COPY(result);
	}
	result[_n]='\0';
	return (int)(_p - start);
}
static int structured_value(PARSER, char *result, int size)
{
	int depth=0;
	char *start=_p;
	START;
	if (*_p!='{') return 0;
	while (size>1 && *_p!='\0' && !(*_p=='}'&&depth==1) ) 
	{
		if ( _p[0]=='\\' && _p[1]!='\0' ) _p++; 
		else if ( *_p=='{' ) depth++; 
		else if ( *_p=='}' ) depth--;
		COPY(result);
	}
	COPY(result);
	result[_n]='\0';
	return (int)(_p - start);
}
static int value(PARSER, char *result, int size)
{
	/* everything to a semicolon */
	char delim=';';
	char *start=_p;
	int quote=0;
	START;
	if ( *_p=='{' ) 
		return structured_value(_p,result,size);
	while (size>1 && *_p!='\0' && !(*_p==delim && quote == 0) && *_p!='\n') 
	{
		if ( _p[0]=='\\' && _p[1]!='\0' )
		{
			_p++; COPY(result);
		}
		else if (*_p=='"')
		{
			*_p++;
			size--;
			quote = (1+quote) % 2;
		}
		else
			COPY(result);
	}
	result[_n]='\0';
	if (quote&1)
		output_warning("%s(%d): missing closing double quote", filename, linenum);
	return (int)(_p - start);
}

#if 0
static int functional_int(PARSER, int64 *value){
	char result[256];
	int size=sizeof(result);
	double pValue;
	char32 fname;
	START;
//	while (size>1 && isdigit(*_p)) COPY(result);
//	result[_n]='\0';
//	*value=atoi64(result);
	/* copy-pasted from functional */
	if (LITERAL("random.") && TERM(name(HERE,fname,sizeof(fname))))
	{
		RANDOMTYPE rtype = random_type(fname);
		int nargs = random_nargs(fname);
		double a;
		if (rtype==RT_INVALID || nargs==0 || (WHITE,!LITERAL("(")))
		{
			output_message("%s(%d): %s is not a valid random distribution", filename,linenum,fname);
			REJECT;
		}
		if (nargs==-1)
		{
			if (WHITE,TERM(real_value(HERE,&a)))
			{
				double b[1024];
				int maxb = sizeof(b)/sizeof(b[0]);
				int n;
				b[0] = a;
				for (n=1; n<maxb && (WHITE,LITERAL(",")); n++)
				{
					if (WHITE,TERM(real_value(HERE,&b[n])))
						continue;
					else
					{
						// variable arg list
						output_message("%s(%d): expected a %s distribution term after ,", filename,linenum, fname);
						REJECT;
					}
				}
				if (WHITE,LITERAL(")"))
				{
					pValue = random_value(rtype,n,b);
					ACCEPT;
				}
				else
				{
					output_message("%s(%d): missing ) after %s distribution terms", filename,linenum, fname);
					REJECT;
				}
			}
			else
			{
				output_message("%s(%d): expected first term of %s distribution", filename,linenum, fname);
				REJECT;
			}
		}
		else 
		{
			if (WHITE,TERM(real_value(HERE,&a)))
			{
				// fixed arg list
				double b,c;
				if (nargs==1)
				{
					if (WHITE,LITERAL(")"))
					{
						pValue = random_value(rtype,a);
						ACCEPT;
					}
					else
					{
						output_message("%s(%d): expected ) after %s distribution term", filename,linenum, fname);
						REJECT;
					}
				}
				else if (nargs==2)
				{
					if ( (WHITE,LITERAL(",")) && (WHITE,TERM(real_value(HERE,&b))) && (WHITE,LITERAL(")")))
					{
						pValue = random_value(rtype,a,b);
						ACCEPT;
					}
					else
					{
						output_message("%s(%d): missing second %s distribution term and/or )", filename,linenum, fname);
						REJECT;
					}
				}
				else if (nargs==3)
				{
					if ( (WHITE,LITERAL(",")) && (WHITE,TERM(real_value(HERE,&b))) && WHITE,LITERAL(",") && (WHITE,TERM(real_value(HERE,&c))) && (WHITE,LITERAL(")")))
					{
						pValue = random_value(rtype,a,b,c);
						ACCEPT;
					}
					else
					{
						output_message("%s(%d): missing terms and/or ) in %s distribution ", filename,linenum, fname);
						REJECT;
					}
				}
				else
				{
					output_message("%s(%d): %d terms is not supported", filename,linenum, nargs);
					REJECT;
				}
			}
			else
			{
				output_message("%s(%d): expected first term of %s distribution", filename,linenum, fname);
				REJECT;
			}
		}
	} // end if "random."
	return _n;
}
#endif

static int integer(PARSER, int64 *value)
{
	char result[256];
	int size=sizeof(result);
	START;
	while (size>1 && isdigit(*_p)) COPY(result);
	result[_n]='\0';
	*value=atoi64(result);
	return _n;
}

static int integer32(PARSER, int32 *value)
{
	char result[256];
	int size=sizeof(result);
	START;
	while (size>1 && isdigit(*_p)) COPY(result);
	result[_n]='\0';
	*value=atoi(result);
	return _n;
}

static int integer16(PARSER, int16 *value)
{
	char result[256];
	int size=sizeof(result);
	START;
	while (size>1 && isdigit(*_p)) COPY(result);
	result[_n]='\0';
	*value=atoi(result);
	return _n;
}

static int real_value(PARSER, double *value)
{
	char result[256];
	int ndigits=0;
	int size=sizeof(result);
	START;
	if (*_p=='+' || *_p=='-') COPY(result);
	while (size>1 && isdigit(*_p)) {COPY(result);++ndigits;}
	if (*_p=='.') COPY(result);
	while (size>1 && isdigit(*_p)) {COPY(result);ndigits++;}
	if (ndigits>0 && (*_p=='E' || *_p=='e')) 
	{
		COPY(result);
		if (*_p=='+' || *_p=='-') COPY(result);
		while (size>1 && isdigit(*_p)) COPY(result);
	}
	result[_n]='\0';
	*value=atof(result);
	return _n;
}

static int functional(PARSER, double *pValue)
{
	char32 fname;
	START;
	if WHITE ACCEPT;
	if (LITERAL("random.") && TERM(name(HERE,fname,sizeof(fname))))
	{
		RANDOMTYPE rtype = random_type(fname);
		int nargs = random_nargs(fname);
		double a;
		if (rtype==RT_INVALID || nargs==0 || (WHITE,!LITERAL("(")))
		{
			output_error_raw("%s(%d): %s is not a valid random distribution", filename,linenum,fname);
			REJECT;
		}
		if (nargs==-1)
		{
			if (WHITE,TERM(real_value(HERE,&a)))
			{
				double b[1024];
				int maxb = sizeof(b)/sizeof(b[0]);
				int n;
				b[0] = a;
				for (n=1; n<maxb && (WHITE,LITERAL(",")); n++)
				{
					if (WHITE,TERM(real_value(HERE,&b[n])))
						continue;
					else
					{
						// variable arg list
						output_error_raw("%s(%d): expected a %s distribution term after ,", filename,linenum, fname);
						REJECT;
					}
				}
				if (WHITE,LITERAL(")"))
				{
					*pValue = random_value(rtype,n,b);
					ACCEPT;
				}
				else
				{
					output_error_raw("%s(%d): missing ) after %s distribution terms", filename,linenum, fname);
					REJECT;
				}
			}
			else
			{
				output_error_raw("%s(%d): expected first term of %s distribution", filename,linenum, fname);
				REJECT;
			}
		}
		else 
		{
			if (WHITE,TERM(real_value(HERE,&a)))
			{
				// fixed arg list
				double b,c;
				if (nargs==1)
				{
					if (WHITE,LITERAL(")"))
					{
						*pValue = random_value(rtype,a);
						ACCEPT;
					}
					else
					{
						output_error_raw("%s(%d): expected ) after %s distribution term", filename,linenum, fname);
						REJECT;
					}
				}
				else if (nargs==2)
				{
					if ( (WHITE,LITERAL(",")) && (WHITE,TERM(real_value(HERE,&b))) && (WHITE,LITERAL(")")))
					{
						*pValue = random_value(rtype,a,b);
						ACCEPT;
					}
					else
					{
						output_error_raw("%s(%d): missing second %s distribution term and/or )", filename,linenum, fname);
						REJECT;
					}
				}
				else if (nargs==3)
				{
					if ( (WHITE,LITERAL(",")) && (WHITE,TERM(real_value(HERE,&b))) && WHITE,LITERAL(",") && (WHITE,TERM(real_value(HERE,&c))) && (WHITE,LITERAL(")")))
					{
						*pValue = random_value(rtype,a,b,c);
						ACCEPT;
					}
					else
					{
						output_error_raw("%s(%d): missing terms and/or ) in %s distribution ", filename,linenum, fname);
						REJECT;
					}
				}
				else
				{
					output_error_raw("%s(%d): %d terms is not supported", filename,linenum, nargs);
					REJECT;
				}
			}
			else
			{
				output_error_raw("%s(%d): expected first term of %s distribution", filename,linenum, fname);
				REJECT;
			}
		}
	} else if TERM(real_value(HERE,pValue)){
		ACCEPT;
	} else
	{
		/* possibly valid through expression() -MH */
		//output_message("%s(%d): expected property or functional value", filename,linenum);
		REJECT;
	}
	DONE;
}

/* Expression rules:
 *	every value is either a double value, or a PT_double object property of the form "this.propname"
 *	valid operators are {+, -, *, /, ^}
 *	every expression begins and ends with parenthesis
 *	every value is followed by an operator or a close parenthesis
 *	every operator is followed by a value or an open parenthesis
 *	every open parenthesis is followed by a value
 *	every close parenthesis is followed by an operator or the end of the expression
 *	parenthesis must be matched
 *	every value or operator must consume trailing whitespace
 * Step one: form the expression list
 * Step two: break the list into a tree
 * Step three: evaluate the tree bottom-up
 *
 *
 *	Dijkstra's Shunting Yard algorithm
 *	ref: wikipedia (sadly)
 *
 *	- read a token
 *	- if the token is a number, add it to the output queue
 *	- if the token is a function token, then push it onto the stack
 *	- if the token is an operator, o1, then:
 *		- while there is an operator, o2, at the top of the stack, and either
 *			- o1 is associative or left-associative and its precedence is less than or equal to that of o2, or
 *			- o1 is right-associative and its precedence is less than that of o2
 *			then pop o2 off the stack and onto the output queue
 *	- if the token is a left parenthesis, then push it onto the stack.
 *	- if the toekn is a right parenthesis:
 *		- until the token at the top is a left parenthesis, pop operators off the stack onto the output queue
 *		- pop the left parenthesis from the stack, but not onto the output queue
 *		- if the token at the top of the stack is a function token, pop it and onto the output queue
 *		- if the stack runs out without finding a left parenthesis, then there are mismatched parentheses
 *	- when there are no more tokens to read:
 *		- while there are still operator tokens on the stack,
 *			- if the operator token is a parenthesis, we have a mismatched parenthesis
 *			- pop the operator onto the queue
 * - FIN
 */
struct s_rpn {
	int op;
	double val; // if op = 0, check val
};

struct s_rpn_func {
	char *name;
	int args; /* use a mode instead? else assume only doubles */
	int index;
	double (*fptr)(double);
	/* fptr? for now, just to recognize */
} rpn_map[] = {
	{"sin", 1, -1, sin},
	{"cos", 1, -2, cos},
	{"tan", 1, -3, tan},
	{"abs", 1, -4, fabs},
	{"sqrt", 1, -5, sqrt},
	{"acos", 1, -6, acos},
	{"asin", 1, -7, asin},
	{"atan", 1, -8, atan},
//	{"atan2", 2},	/* only one with two inputs? */
	{"log", 1, -10, log},
	{"log10", 1, -11, log10},
	{"floor", 1, -12, floor},
	{"ceil", 1, -13, ceil}
};

static int rpnfunc(PARSER, int *val){
	struct s_rpn_func *ptr = rpn_map;
	int i = 0, count = 0;
	START;
	count = (sizeof(rpn_map)/sizeof(rpn_map[0]));
	for(i = 0; i < count; ++i){
		if(strncmp(rpn_map[i].name, HERE, strlen(rpn_map[i].name)) == 0){
			*val = rpn_map[i].index;
			return (int)strlen(rpn_map[i].name);
		}
	}
	return 0;
}

//static const int OP_END = 0, OP_OPEN = 1, OP_CLOSE = 2, OP_POW = 3,
//		OP_MULT = 4, OP_MOD = 5, OP_DIV = 6, OP_ADD = 7, OP_SUB = 8;
//static int OP_SIN = -1, OP_COS = -2, OP_TAN = -3, OP_ABS = -4;

#define OP_END 0
#define OP_OPEN 1
#define OP_CLOSE 2
#define OP_POW 3
#define OP_MULT 4
#define OP_MOD 5
#define OP_DIV 6
#define OP_ADD 7
#define OP_SUB 8
#define OP_SIN -1
#define OP_COS -2
#define OP_TAN -3
#define OP_ABS -4

static int op_prec[] = {0, 0, 0, 3, 2, 2, 2, 1, 1};

#define PASS_OP(T) \
	while(op_prec[(T)] <= op_prec[op_stk[op_i]]){	\
		rpn_stk[rpn_i].op = op_stk[op_i];				\
		rpn_stk[rpn_i].val = 0;							\
		++rpn_i;										\
		--op_i;											\
	}													\
	op_stk[++op_i] = (T);							\
	++rpn_sz;							
	
static int expression(PARSER, double *pValue, UNIT **unit, OBJECT *obj){
	double val_q[128], tVal;
	char tname[128]; /* type name for this.prop */
	char oname[128], pname[128];
	struct s_rpn rpn_stk[256];
	int op_stk[128], val_i = 0, op_i = 1, rpn_i = 0, depth = 0, rfname = 0, rpn_sz = 0;
	int i = 0;
	
	START;
	/* RPN-ify */
	if LITERAL("("){
		ACCEPT;
		if WHITE ACCEPT;
		depth = 1;
		op_stk[0] = OP_OPEN;
		op_i = 0;
	} else {
		REJECT; /* all expressions must be contained within a () block */
	}
	while(depth > 0){ /* grab tokens*/
		if LITERAL(";"){ /* says we're done */
			ACCEPT;
			break;
		} else if LITERAL("("){ /* parantheses */
			ACCEPT;
			op_stk[++op_i] = OP_OPEN;
			//++op_i;
			++depth;
			if WHITE ACCEPT;
		} else if LITERAL(")"){
			ACCEPT;
			if WHITE ACCEPT;
			--depth;
			/* consume operations until OP_OPEN found */
			while((op_i >= 0) && (op_stk[op_i] != OP_OPEN)){
				rpn_stk[rpn_i].op = op_stk[op_i--];
				rpn_stk[rpn_i].val = 0.0;
				++rpn_i;
			}
			/* consume OP_OPEN too */
			op_i--;
			/* rpnfunc lookahead */
			if(op_stk[op_i] < 0){ /* push rpnfunc */
				rpn_stk[rpn_i].op = op_stk[op_i--];
				rpn_stk[rpn_i].val = 0.0;
				++rpn_i;
			}
			/* op_stk[op_i] == OP_CLOSE */
		} else if LITERAL("^"){ /* operators */
			ACCEPT;
			if WHITE ACCEPT;
			op_stk[++op_i] = OP_POW; /* nothing but () and functions hold higher precedence */
			++rpn_sz;
		} else if LITERAL("*"){ /* prec = 4 */
			ACCEPT;
			if WHITE ACCEPT;
			PASS_OP(OP_MULT);
		} else if LITERAL("/"){
			ACCEPT;
			if WHITE ACCEPT;
			PASS_OP(OP_DIV);
		} else if LITERAL("%"){
			ACCEPT;
			if WHITE ACCEPT;
			PASS_OP(OP_MOD);
		} else if LITERAL("+"){ /* prec = 6 */
			ACCEPT;
			if WHITE ACCEPT;
			PASS_OP(OP_ADD);
		} else if LITERAL("-"){
			ACCEPT;
			if WHITE ACCEPT;
			PASS_OP(OP_SUB);
		} else if(TERM(rpnfunc(HERE, &rfname))){
			ACCEPT;
			if WHITE ACCEPT;
			op_stk[++op_i] = rfname;
			if LITERAL("("){
				ACCEPT;
				if WHITE ACCEPT;
				op_stk[++op_i] = OP_OPEN;
				++depth;
				++rpn_sz;
			} else {
				REJECT;
			}
		} else if ((LITERAL("$") || LITERAL("this.")) && TERM(name(HERE,tname,sizeof(tname)))){
			/* TODO: support non-this object name references. would need to delay processing of expr
			 * with unrecognized object names. -d3p988 */
			double *valptr = object_get_double_by_name(obj, tname);
			if(valptr == NULL){
				output_error_raw("%s(%d): invalid property: %s.%s", filename,linenum, obj->oclass->name, tname);
				REJECT;
			}
			ACCEPT;
			if WHITE ACCEPT;
			rpn_stk[rpn_i].op = 0;
			rpn_stk[rpn_i].val = *valptr;
			++rpn_sz;
			++rpn_i;
		} else if (TERM(functional(HERE, &tVal))){ /* captures reals too */
			ACCEPT;
			if WHITE ACCEPT;
			rpn_stk[rpn_i].op = 0;

			rpn_stk[rpn_i].val = tVal;
			++rpn_i;
			++rpn_sz;
		} else if(TERM(name(HERE,oname,sizeof(oname))) && LITERAL(".") && TERM(name(HERE,pname,sizeof(pname)))){
			/* obj.prop*/
			OBJECT *otarg = NULL;
			ACCEPT;
			if WHITE ACCEPT;
			if(0 == strcmp(oname, "parent")){
				otarg = obj->parent;
			} else {
				otarg = object_find_name(oname);
			}
			if(otarg == NULL){ // delayed checking
				// disabled for now
				output_error_raw("%s(%d): unknown reference: %s.%s", filename, linenum, oname, pname);
				output_error("may be an order issue, delayed reference checking is a todo");
				REJECT;
			} else {
				double *valptr = object_get_double_by_name(otarg, pname);
				if(valptr == NULL){
					output_error_raw("%s(%d): invalid property: %s.%s", filename,linenum, oname, pname);
					REJECT;
				}
				rpn_stk[rpn_i].op = 0;
				rpn_stk[rpn_i].val = *valptr;
				++rpn_sz;
				++rpn_i;
			}
		} else { /* oops */
			output_error_raw("%s(%d): unrecognized token within: %s9", filename,linenum, HERE-2);
			REJECT;
			/* It looked like an expression.  Give fair warning. */
		}
	}

	/* depth == 0 ~ pop the op stack to the rpn queue */
	while(op_i >= 0){
		if(op_stk[op_i] != OP_OPEN){
			rpn_stk[rpn_i].op = op_stk[op_i];
			rpn_stk[rpn_i].val = 0.0;
			++rpn_i;
		}
		--op_i;
	}
	/* if no semicolon, there's a bigger error, so we don't check that here */
	
	/* postfix algorithm */
	/*	- while there are input tokens left,
	 *		- read the next input token
	 *		- if the token is a value
	 *			- push the token onto a stack
	 *		- if the token is an operator
	 *			- it is known a priori that the operator takes N arguments
	 *			- if there are fewer than N values on the stack, error.
	 *			- else pop the top n values from the stack
	 *			- evaluate the operator with with the values as arguments
	 *			- push the returned value back onto the stack
	 *	- iff one value remains on the stack, return that value
	 *	- if more values exist on the stack, error
	 */

	rpn_i = 0;

	for(i = 0; i < rpn_sz; ++i){
		if(rpn_stk[i].op == 0){ /* push value */
			val_q[val_i++] = rpn_stk[i].val;
		} else if(rpn_stk[i].op > 0){ /* binary operator */
			double popval = val_q[--val_i];
			if(val_i < 0){
				output_error_raw("%s(%d): insufficient arguments in equation", filename,linenum, rpn_stk[i].op);
				REJECT;
			}
			switch(rpn_stk[i].op){
				case OP_POW:
					val_q[val_i-1] = pow(val_q[val_i-1], popval);
					break;
				case OP_MULT:
					val_q[val_i-1] *= popval;
					break;
				case OP_MOD:
					val_q[val_i-1] = fmod(val_q[val_i-1], popval);
					break;
				case OP_DIV:
					val_q[val_i-1] /= popval;
					break;
				case OP_ADD:
					val_q[val_i-1] += popval;
					break;
				case OP_SUB:
					val_q[val_i-1] -= popval;
					break;
				default:
					output_error_raw("%s(%d): unrecognized operator index %i (bug!)", filename,linenum, rpn_stk[i].op);
					REJECT;
			}
		} else if(rpn_stk[i].op < 0){ /* rpn_func */
			int j;
			int count = (sizeof(rpn_map)/sizeof(rpn_map[0]));
			for(j = 0; j < count; ++j){
				if(rpn_map[j].index == rpn_stk[i].op){
					double popval = val_q[--val_i];
					if(val_i < 0){
						output_error_raw("%s(%d): insufficient arguments in equation", filename,linenum, rpn_stk[i].op);
						REJECT;
					}
					val_q[val_i++] = (*rpn_map[j].fptr)(popval);
					break;
				}
			}
			if(j == count){ /* missed */
				output_error_raw("%s(%d): unrecognized function index %i (bug!)", filename,linenum, rpn_stk[i].op);
				REJECT;
			}

		}
	}
	if((val_i > 1)){
		output_error_raw("%s(%d): too many values in equation!", filename,linenum);
		REJECT;
	}
	*pValue = val_q[0];
	DONE;
}

static int functional_unit(PARSER,double *pValue,UNIT **unit)
{
	START;
	if TERM(functional(HERE,pValue))
	{
		*unit = NULL;
		if WHITE ACCEPT;
		if TERM(unitspec(HERE,unit)) ACCEPT;
		ACCEPT;
		DONE;
	}
	REJECT;
}

static int complex_value(PARSER, complex *pValue)
{
	double r, i, m, a;
	START;
	if ((WHITE,TERM(real_value(HERE,&r))) && (WHITE,TERM(real_value(HERE,&i))) && LITERAL("i"))
	{
		pValue->r = r;
		pValue->i = i;
		pValue->f = I;
		ACCEPT;
		DONE;
	}
	OR
	if ((WHITE,TERM(real_value(HERE,&r))) && (WHITE,TERM(real_value(HERE,&i))) && LITERAL("j"))
	{
		pValue->r = r;
		pValue->i = i;
		pValue->f = J;
		ACCEPT;
		DONE;
	}
	OR
	if ((WHITE,TERM(real_value(HERE,&m))) && (WHITE,TERM(real_value(HERE,&a))) && LITERAL("d"))
	{
		pValue->r = m*cos(a*PI/180);
		pValue->i = m*sin(a*PI/180);
		pValue->f = A;
		ACCEPT;
		DONE;
	}
	OR
	if ((WHITE,TERM(real_value(HERE,&m))) && (WHITE,TERM(real_value(HERE,&a))) && LITERAL("r"))
	{
		pValue->r = m*cos(a);
		pValue->i = m*sin(a);
		pValue->f = R;
		ACCEPT;
		DONE;
	} 
	OR
	if ((WHITE,TERM(real_value(HERE,&m))))
	{
		pValue->r = m;
		pValue->i = 0.0;
		pValue->f = I;
		ACCEPT;
		DONE;
	}

	REJECT;
}

static int complex_unit(PARSER,complex *pValue,UNIT **unit)
{
	START;
	if TERM(complex_value(HERE,pValue))
	{
		*unit = NULL;
		if WHITE ACCEPT;
		if TERM(unitspec(HERE,unit)) ACCEPT;
		ACCEPT;
		DONE;
	}
	REJECT;
}

static int time_value_seconds(PARSER, TIMESTAMP *t)
{
	START;
	if WHITE ACCEPT;
	if (TERM(integer(HERE,t)) && LITERAL("s")) { *t *= TS_SECOND; ACCEPT; DONE;}
	OR
	if (TERM(integer(HERE,t)) && LITERAL("S")) { *t *= TS_SECOND; ACCEPT; DONE;}
	REJECT;
}

static int time_value_minutes(PARSER, TIMESTAMP *t)
{
	START;
	if WHITE ACCEPT;
	if (TERM(integer(HERE,t)) && LITERAL("m")) { *t *= 60*TS_SECOND; ACCEPT; DONE;}
	OR
	if (TERM(integer(HERE,t)) && LITERAL("M")) { *t *= 60*TS_SECOND; ACCEPT; DONE;}
	REJECT;
}

static int time_value_hours(PARSER, TIMESTAMP *t)
{
	START;
	if WHITE ACCEPT;
	if (TERM(integer(HERE,t)) && LITERAL("h")) { *t *= 3600*TS_SECOND; ACCEPT; DONE;}
	OR
	if (TERM(integer(HERE,t)) && LITERAL("H")) { *t *= 3600*TS_SECOND; ACCEPT; DONE;}
	REJECT;
}

static int time_value_days(PARSER, TIMESTAMP *t)
{
	START;
	if WHITE ACCEPT;
	if (TERM(integer(HERE,t)) && LITERAL("d")) { *t *= 86400*TS_SECOND; ACCEPT; DONE;}
	OR
	if (TERM(integer(HERE,t)) && LITERAL("D")) { *t *= 86400*TS_SECOND; ACCEPT; DONE;}
	REJECT;
}

int time_value_datetime(PARSER, TIMESTAMP *t)
{
	DATETIME dt;
	START;
	if WHITE ACCEPT;
	if LITERAL("'") ACCEPT;
	if (TERM(integer16(HERE,&dt.year)) && LITERAL("-")
		&& TERM(integer16(HERE,&dt.month)) && LITERAL("-")
		&& TERM(integer16(HERE,&dt.day)) && LITERAL(" ")
		&& TERM(integer16(HERE,&dt.hour)) && LITERAL(":")
		&& TERM(integer16(HERE,&dt.minute)) && LITERAL(":")
		&& TERM(integer16(HERE,&dt.second)) && LITERAL("'"))
	{
		dt.nanosecond = 0;
		dt.weekday = -1;
		dt.is_dst = -1;
		strcpy(dt.tz,"");
		*t = mkdatetime(&dt);
		if (*t!=-1) 
		{
			ACCEPT;
		}
		else
			REJECT;
	}
	else
		REJECT;
	DONE;
}

int time_value_datetimezone(PARSER, TIMESTAMP *t)
{
	DATETIME dt;
	START;
	if WHITE ACCEPT;
	if (LITERAL("'")||LITERAL("\"")) ACCEPT;
	if (TERM(integer16(HERE,&dt.year)) && LITERAL("-")
		&& TERM(integer16(HERE,&dt.month)) && LITERAL("-")
		&& TERM(integer16(HERE,&dt.day)) && LITERAL(" ")
		&& TERM(integer16(HERE,&dt.hour)) && LITERAL(":")
		&& TERM(integer16(HERE,&dt.minute)) && LITERAL(":")
		&& TERM(integer16(HERE,&dt.second)) && LITERAL(" ")
		&& TERM(name(HERE,dt.tz,sizeof(dt.tz))) && (LITERAL("'")||LITERAL("\"")))
	{
		dt.nanosecond = 0;
		dt.weekday = -1;
		dt.is_dst = -1;
		*t = mkdatetime(&dt);
		if (*t!=-1) 
		{
			ACCEPT;
		}
		else
			REJECT;
	}
	else
		REJECT;
	DONE;
}

/*static*/ int time_value(PARSER, TIMESTAMP *t)
{
	START;
	if WHITE ACCEPT;
	if (TERM(time_value_seconds(HERE,t)) && (WHITE,LITERAL(";"))) {ACCEPT; DONE; }
	OR
	if (TERM(time_value_minutes(HERE,t)) && (WHITE,LITERAL(";"))) {ACCEPT; DONE; }
	OR
	if (TERM(time_value_hours(HERE,t)) && (WHITE,LITERAL(";"))) {ACCEPT; DONE; }
	OR
	if (TERM(time_value_days(HERE,t)) && (WHITE,LITERAL(";"))) {ACCEPT; DONE; }
	OR
	if (TERM(time_value_datetime(HERE,t)) && (WHITE,LITERAL(";"))) {ACCEPT; DONE; }
	OR
	if (TERM(time_value_datetimezone(HERE,t)) && (WHITE,LITERAL(";"))) {ACCEPT; DONE; }
	OR
	if (TERM(integer(HERE,t)) && (WHITE,LITERAL(";"))) {ACCEPT; DONE; }
	else REJECT;
	DONE;
}

double load_latitude(char *buffer)
{
	double d=0, m=0, s=0;
	char ns;
	if((strcmp(buffer, "none") == 0) || (strcmp(buffer, "NONE") == 0)){
		return QNAN;
	}
	if (sscanf(buffer,"%lf%c%lf:%lf",&d,&ns,&m,&s)>1)
	{
		double v = d+m/60+s/3600;
		if (v>=0 && v<=90)
		{
			if (ns=='S')
				return -v;
			else if (ns=='N')
				return v;
		}
	}
	output_error_raw("%s(%d): '%s' is not a valid latitude", filename,linenum,buffer);
	return QNAN;
}

double load_longitude(char *buffer)
{
	double d=0, m=0, s=0;
	char ns;
	if((strcmp(buffer, "none") == 0) || (strcmp(buffer, "NONE") == 0)){
		return QNAN;
	}
	if (sscanf(buffer,"%lf%c%lf:%lf",&d,&ns,&m,&s)>1)
	{
		double v = (double)d+(double)m/60+s/3600;
		if (v>=0 && v<=180)
		{
			if (ns=='W')
				return -v;
			else if (ns=='E')
				return v;
		}
	}
	output_error_raw("%s(%d): '%s' is not a valid longitude", filename,linenum,buffer);
	return QNAN;
}

static int clock_properties(PARSER)
{
	TIMESTAMP tsval;
	char32 timezone;
	double realval;
	START;
	if WHITE ACCEPT;
	if (LITERAL("tick") && WHITE)
	{
		if (TERM(real_value(HERE,&realval)) && (WHITE,LITERAL(";")))
		{
			if (realval!=TS_RESOLUTION)
			{
				output_error_raw("%s(%d): timestamp resolution %g does not match system resolution %g, this version does not support variable tick", filename, linenum, realval, TS_RESOLUTION);
				REJECT;
			}
			ACCEPT;
			goto Next;
		}
		output_error_raw("%s(%d): expected tick value", filename, linenum);
		REJECT;
	}
	OR if (LITERAL("timestamp") && WHITE)
	{
		if (TERM(time_value(HERE,&tsval)))
		{
			//global_clock = tsval;
			global_starttime = tsval; // used to affect start time, before with 
			ACCEPT;
			goto Next;
		}
		output_error_raw("%s(%d): expected time value", filename, linenum);
		REJECT;
	}
	OR if (LITERAL("starttime") && WHITE)
	{
		if (TERM(time_value(HERE,&tsval)))
		{
			global_starttime = tsval;
			ACCEPT;
			goto Next;
		}
		output_error_raw("%s(%d): expected time value", filename, linenum);
		REJECT;
	}
	OR if (LITERAL("stoptime") && WHITE)
	{
		if (TERM(time_value(HERE,&tsval)))
		{
			global_stoptime = tsval;
			ACCEPT;
			goto Next;
		}
		output_error_raw("%s(%d): expected time value", filename, linenum);
		REJECT;
	}
	OR if (LITERAL("timezone") && WHITE)
	{
		if (TERM(value(HERE,timezone,sizeof(timezone))) && (WHITE,LITERAL(";")) && strlen(timezone)>0)
		{
			if (timestamp_set_tz(timezone)==NULL)
				output_warning("%s(%d): timezone %s is not defined",filename,linenum,timezone);
				/* TROUBLESHOOT
					The specified timezone is not defined in the timezone file <code>.../etc/tzinfo.txt</code>.  
					Try using an known timezone, or add the desired timezone to the timezome file and try again.
				 */
			ACCEPT;
			goto Next;
		}
		output_error_raw("%s(%d): expected time zone specification", filename, linenum);
		REJECT;
	}
	OR if (WHITE,LITERAL("}")) {/* don't accept yet */ DONE;}
	OR { syntax_error(HERE); REJECT; }
	/* may be repeated */
Next:
	if TERM(clock_properties(HERE)) ACCEPT;
	DONE;
}

static int pathname(PARSER, char *path, int size)
{
	START;
	if TERM(pattern(HERE,"[-A-Za-z0-9/\\:_,. ]",path,size)) {ACCEPT;}
	else REJECT;
	DONE;
}

/** Expanded values support in-place expansion of special context sensitive variables.
	Expanded values are enclosed in backquotes. The variables are specified using the 
	{varname} syntax.  The following variables are supported:

	{file} embeds the current file (full path,name,extension)
	{filename} embeds the name of the file (no path, no extension)
	{fileext} embeds the extension of the file (no path, no name)
	{filepath} embeds the path of the file (no name, no extension)
	{line} embeds the current line number
	{namespace} embeds the name of the current namespace
	{class}	embeds the classname of the current object
	{id} embeds the id of the current object
	{var} embeds the current value of the current object's variable <var>

 **/
static OBJECT *current_object = NULL; /* context object */
static MODULE *current_module = NULL; /* context module */
static int expanded_value(char *text, char *result, int size, char *delims)
{
	int n=0;
	if (text[n] == '`')
	{
		n++;
		memset(result,0,size--); /* preserve the string terminator even when buffer is full */
		for ( ; text[n]!='`'; n++)
		{
			if (size==0)
			{
				output_error_raw("%s(%d): string expansion buffer overrun", filename, linenum);
				return 0;
			}
			if (text[n]=='{')
			{
				char varname[256];
				char value[1024];
				char path[1024], name[1024], ext[1024];
				filename_parts(filename,path,name,ext);

				if (sscanf(text+n+1,"%255[a-zA-Z0-9_:]",varname)==0)
				{
					output_error_raw("%s(%d): expanded string variable syntax error", filename, linenum);
					return 0;
				}
				n+=(int)strlen(varname)+1;
				if (text[n]!='}')
				{
					output_error_raw("%s(%d): expanded string variable missing closing }", filename, linenum);
					return 0;
				}

				/* expanded specials variables */
				if (strcmp(varname,"file")==0)
					strcpy(value,filename);
				else if (strcmp(varname,"filename")==0)
					strcpy(value,name);
				else if (strcmp(varname,"filepath")==0)
					strcpy(value,path); 
				else if (strcmp(varname,"fileext")==0)
					strcpy(value,ext);
				else if (strcmp(varname,"namespace")==0)
					object_namespace(value,sizeof(value));
				else if (strcmp(varname,"class")==0)
					strcpy(value,current_object?current_object->oclass->name:"");
				else if (strcmp(varname,"gridlabd")==0)
					strcpy(value,global_execdir);
				else if (strcmp(varname,"hostname")==0)
					strcpy(value,global_hostname); 
				else if (strcmp(varname,"hostaddr")==0)
					strcpy(value,global_hostaddr); 
				else if (strcmp(varname,"cpu")==0)
					sprintf(value,"%d",sched_get_cpuid(0)); 
				else if (strcmp(varname,"pid")==0)
					sprintf(value,"%d",sched_get_procid()); 
				else if (strcmp(varname,"port")==0)
					sprintf(value,"%d",global_server_portnum);
				else if (strcmp(varname,"mastername")==0)
					strcpy(value,"localhost"); /* @todo copy actual master name */
				else if (strcmp(varname,"masteraddr")==0)
					strcpy(value,"127.0.0.1"); /* @todo copy actual master addr */
				else if (strcmp(varname,"masterport")==0)
					strcpy(value,"6267"); /* @todo copy actual master port */
				else if (strcmp(varname,"id")==0)
				{
					if (current_object)
						sprintf(value,"%d",current_object->id);
					else
						strcpy(value,"");
				}
				else if ( object_get_value_by_name(current_object,varname,value,sizeof(value)))
				{
					/* value is ok */
				}
				else if ( global_getvar(varname,value,sizeof(value)) )
				{
					/* value is ok */
				}
				else
				{
					output_error_raw("%s(%d): variable '%s' not found in this context", filename, linenum, varname);
					return 0;
				}

				/* accept the value */
				if ((int)strlen(value)>=size)
				{
					output_error_raw("%s(%d): string expansion buffer overrun", filename, linenum);
					return 0;
				}
				strcat(result,value);
				size -= (int)strlen(value);
				result += strlen(value);
			}
			else
			{
				*result++ = text[n];
				size--;
			}
		}
		if (text[n+1]==';')
			return n+1;
		else
		{
			output_error_raw("%s(%d): missing terminating ;", filename, linenum);
			return 0;
		}
	}
	else if (delims==NULL)
		return value(text,result,size);
	else
		return delim_value(text,result,size,delims);
}

/** alternate_value allows the use of ternary operations, e.g.,

		 property (expression) ? negzero_value : positive_value ;

 **/

static int alternate_value(PARSER, char *value, int size)
{
	double test;
	char value1[1024];
	char value2[1024];
	START;
	if (WHITE) ACCEPT;
	if (TERM(expression(HERE,&test,NULL,current_object)) && (WHITE,LITERAL("?")))
	{
		if ((WHITE,TERM(expanded_value(HERE,value1,sizeof(value1)," \t\n:"))) && (WHITE,LITERAL(":")) && (WHITE,TERM(expanded_value(HERE,value2,sizeof(value2)," \n\t;"))))
		{
			ACCEPT;
			if (test>0)
			{
				if ((int)strlen(value1)>size)
				{
					output_error_raw("%s(%d): alternate value 1 is too large ;", filename, linenum);
					REJECT;
				}
				else
				{
					strcpy(value,value1);
					ACCEPT;
				}
			}
			else
			{
				if ((int)strlen(value2)>size)
				{
					output_error_raw("%s(%d): alternate value 2 is too large ;", filename, linenum);
					REJECT;
				}
				else
				{
					strcpy(value,value2);
					ACCEPT;
				}
			}
		}
		else
		{
			output_error_raw("%s(%d): missing or invalid alternate values;", filename, linenum);
			REJECT;
		}
		DONE;
	}
	OR if (TERM(expanded_value(HERE,value,size,NULL)))
	{
		ACCEPT;
		DONE
	}
	REJECT;
	DONE;
}

/** Line specs are generated internally to maintain proper filename and line number context. 
	Line specs are always alone on a line and take the form @pathname;linenum
 **/
static int line_spec(PARSER)
{
	char fname[1024];
	int32 lnum;
	START;
	if LITERAL("@") 
	{
		if (TERM(pathname(HERE,fname,sizeof(fname))) && LITERAL(";") && TERM(integer32(HERE,&lnum)))
		{
			strcpy(filename,fname);
			linenum = lnum;
			ACCEPT; DONE;
		}
		else
		{
			output_error_raw("%s(%d): @ syntax error", filename, linenum);
			REJECT; DONE;
		}
	}
	else
		REJECT;
	DONE;
}

static int clock_block(PARSER)
{
	START;
	if WHITE ACCEPT;
	if LITERAL("clock") ACCEPT else REJECT;
	if WHITE ACCEPT;
	if LITERAL("{") ACCEPT
	else
	{
		output_error_raw("%s(%d): expected clock block opening {",filename,linenum);
		REJECT;
	}
	if WHITE ACCEPT;
	// cache timestamp for delayed timestamp offsets
	if TERM(clock_properties(HERE)) ACCEPT;
	if WHITE ACCEPT;
	if LITERAL("}") ACCEPT else
	{
		output_error_raw("%s(%d): expected clock block closing }",filename,linenum);
		REJECT;
	}
	if(0 != start_ts[0]){
		;
	}
	if(0 != stop_ts[0]){
		;
	}
	DONE;
}

static int module_properties(PARSER, MODULE *mod)
{
	int64 val;
	CLASSNAME classname;
	char256 propname;
	char256 propvalue;
	START;
	if WHITE ACCEPT;
	if (LITERAL("major") && (WHITE))
	{
		if (TERM(integer(HERE,&val)))
		{
			if WHITE ACCEPT;
			if LITERAL(";")
			{
				if (val!=mod->major)
				{
					output_error_raw("%s(%d): %s has an incompatible module major version", filename,linenum,mod->name);
					REJECT;
				}
				ACCEPT;
				goto Next;
			}
			else
			{
				output_error_raw("%s(%d): expected ; after %s module major number", filename,linenum, mod->name);
				REJECT;
			}
		}
		else
		{
			output_error_raw("%s(%d): expected %s module major number", filename,linenum, mod->name);
			REJECT;
		}
	}
	OR if (LITERAL("minor") && (WHITE))
	{
		if (TERM(integer(HERE,&val)))
		{
			if WHITE ACCEPT;
			if LITERAL(";")
			{
				if (val!=mod->minor)
				{
					output_error_raw("%s(%d): %s has an incompatible module minor version", filename,linenum,mod->name);
					REJECT;
				}
				ACCEPT;
				goto Next;
			}
			else
			{
				output_error_raw("%s(%d): expected ; after %s module minor number", filename,linenum, mod->name);
				REJECT;
			}
		}
		else
		{
			output_error_raw("%s(%d): expected %s module minor number", filename,linenum, mod->name);
			REJECT;
		}
	}
	OR if (LITERAL("class") && WHITE)
	{
		if TERM(name(HERE,classname,sizeof(classname)))
		{
			if WHITE ACCEPT;
			if LITERAL(";")
			{
				CLASS *oclass = class_get_class_from_classname(classname);
				if (oclass==NULL || oclass->module!=mod)
				{
					output_error_raw("%s(%d): module '%s' does not implement class '%s'", filename, linenum, mod->name, classname);
					REJECT;
				}
				ACCEPT;
				goto Next;
			}
			else
			{
				output_error_raw("%s(%d): expected ; after module %s class %s declaration", filename,linenum, mod->name, classname);
				REJECT;
			}
		}
		else
		{
			output_error_raw("%s(%d): missing class name in module %s class declaration", filename,linenum, mod->name);
			REJECT;
		}
	}
	OR if (TERM(name(HERE,propname,sizeof(propname))) && (WHITE))
	{
		current_object = NULL; /* object context */
		current_module = mod; /* module context */
		if TERM(alternate_value(HERE,propvalue,sizeof(propvalue)))
		{
			if WHITE ACCEPT;
			if LITERAL(";")
			{
				if (module_setvar(mod,propname,propvalue)>0)
				{
					ACCEPT;
					goto Next;
				}
				else
				{
					output_error_raw("%s(%d): invalid module %s property '%s'", filename, linenum, mod->name, propname);
					REJECT;
				}
			}
			else
			{
				output_error_raw("%s(%d): expected ; after module %s property specification", filename, linenum, mod->name);
				REJECT;
			}
		}
		else
		{
			output_error_raw("%s(%d): missing module %s property %s value", filename, linenum, mod->name, propname);
			REJECT;
		}
	}
	OR if LITERAL("}") {/* don't accept yet */ DONE;}
	OR { syntax_error(HERE); REJECT; }
	/* may be repeated */
Next:
	if TERM(module_properties(HERE,mod)) ACCEPT;
	DONE;
}

static int module_block(PARSER)
{
	char module_name[64];
	char fmod[8],mod[54];
	MODULE *module;
	START;
	if WHITE ACCEPT;
	if (LITERAL("module") && WHITE) ACCEPT else REJECT;
	//if WHITE ACCEPT;
	/* load options should go here and get converted to argc/argv */

	/* foreign module */
	if (TERM(name(HERE,fmod,sizeof(fmod))) && LITERAL("::") && TERM(name(HERE,mod,sizeof(mod))))
	{
		sprintf(module_name,"%s::%s",fmod,mod);
		if ((module=module_load(module_name,0,NULL))!=NULL)
		{
			ACCEPT;
		}
		else
		{
			output_error_raw("%s(%d): %s module '%s' load failed, %s", filename, linenum, fmod, mod,errno?strerror(errno):"(no details)");
			REJECT;
		}
	}

	OR
	/* native C/C++ module */
	if (TERM(name(HERE,module_name,sizeof(module_name))))
	{
		if ((module=module_load(module_name,0,NULL))!=NULL)
		{
			ACCEPT;
		}
		else
		{
			output_error_raw("%s(%d): module '%s' load failed, %s", filename, linenum, module_name,errno?strerror(errno):"(no details)");
			REJECT;
		}
	}
	if WHITE ACCEPT;
	if LITERAL(";") {ACCEPT;DONE;}
	OR
	if LITERAL("{") ACCEPT
	else
	{
		output_error_raw("%s(%d): expected module %s block opening {", filename, linenum, module_name);
		REJECT;
	}
	if TERM(module_properties(HERE,module)) ACCEPT else REJECT;
	if WHITE ACCEPT;
	if LITERAL("}") ACCEPT
	else
	{
		output_error_raw("%s(%d): expected module %s block closing }", filename, linenum, module_name);
		REJECT;
	}
	DONE;
}

static int property_specs(PARSER, KEYWORD **keys)
{
	char keyname[32];
	int32 keyvalue;
	START;
	if WHITE ACCEPT;
	if ( TERM(name(HERE,keyname,sizeof(keyname))) && (WHITE,LITERAL("=")) && TERM(integer32(HERE,&keyvalue)))
	{
		*keys = malloc(sizeof(KEYWORD));
		(*keys)->next = NULL;
		if WHITE ACCEPT;
		if LITERAL(",") ACCEPT;
		if WHITE ACCEPT;
		if TERM(property_specs(HERE, &((*keys)->next)));
		ACCEPT;
		strcpy((*keys)->name,keyname);
		(*keys)->value = keyvalue;
	}
	else
		REJECT;
	DONE;
}

static int property_type(PARSER, PROPERTYTYPE *ptype, KEYWORD **keys)
{
	char32 type;
	START;
	if WHITE ACCEPT;
	if TERM(name(HERE,type,sizeof(type)))
	{
		*ptype = class_get_propertytype_from_typename(type);
		if (*ptype==PT_void)
		{
			output_error_raw("%s(%d): class member %s is not recognized", filename, linenum, type);
			REJECT;
		}
		if (WHITE,LITERAL("{"))
		{
			if (TERM(property_specs(HERE,keys)) && (WHITE,LITERAL("}")))
			{
				ACCEPT;}
			else
			{	REJECT;}
		}
		ACCEPT;
	}
	else REJECT;
	DONE;
}

static int class_intrinsic_function_name(PARSER, CLASS *oclass, int64 *function, char **ftype, char **fname)
{
	char buffer[1024];
	START;
	if WHITE ACCEPT;
	if LITERAL("create")
	{
		*ftype = "int64";
		*fname = "create";
		*function |= FN_CREATE;
		ACCEPT;
	}
	else if LITERAL("init")
	{
		*ftype = "int64";
		*fname = "init";
		*function |= FN_INIT;
		ACCEPT;
	}
	else if LITERAL("precommit")
	{
		*ftype = "int64";
		*fname = "precommit";
		*function |= FN_PRECOMMIT;
		ACCEPT;
	}
	else if LITERAL("presync")
	{
		*ftype = "TIMESTAMP";
		*fname = "presync";
		oclass->passconfig |= PC_PRETOPDOWN;
		*function |= FN_PRESYNC;
		ACCEPT;
	}
	else if LITERAL("sync")
	{
		*ftype = "TIMESTAMP";
		*fname = "sync";
		oclass->passconfig |= PC_BOTTOMUP;
		*function |= FN_SYNC;
		ACCEPT;
	}
	else if LITERAL("postsync")
	{
		*ftype = "TIMESTAMP";
		*fname = "postsync";
		oclass->passconfig |= PC_POSTTOPDOWN;
		*function |= FN_POSTSYNC;
		ACCEPT;
	}
	else if LITERAL("recalc")
	{
		*ftype = "int64";
		*fname = "recalc";
		*function |= FN_RECALC;
		ACCEPT;
	}
	else if LITERAL("notify")
	{
		*ftype = "int64";
		*fname = "notify";
		*function |= FN_NOTIFY;
		ACCEPT;
	}
	else if LITERAL("plc")
	{
		*ftype = "TIMESTAMP";
		*fname = "plc";
		*function |= FN_PLC;
		ACCEPT;
	}
	else if LITERAL("isa")
	{
		*ftype = "int64";
		*fname = "isa";
		*function |= FN_ISA;
		ACCEPT;
	}
	else if LITERAL("commit")
	{
		*ftype = "TIMESTAMP";
		*fname = "commit";
		*function |= FN_COMMIT;
		ACCEPT;
	}
	else if LITERAL("finalize")
	{
		*ftype = "int64";
		*fname = "finalize";
		*function |= FN_FINALIZE;
		ACCEPT;
	}
	else if TERM(name(HERE,buffer,sizeof(buffer)))
	{
		output_error_raw("%s(%d): '%s' is not a recognized intrinsic function", filename,linenum,buffer);
		REJECT;
	}
	DONE;
}

static int argument_list(PARSER, char *args, int size)
{
	START;
	if WHITE ACCEPT;
	strcpy(args,"");
	if (LITERAL("("))
	{
		if (WHITE,TERM(pattern(HERE,"[^)]",args,size)))
		{
			ACCEPT;
		}
		if (WHITE,LITERAL(")"))
		{
			ACCEPT;
		}
		else
		{
			output_error_raw("%s(%d): unterminated argument list",filename, linenum);
			REJECT;
		}
	}
	else
		REJECT;
	DONE;
}

static int source_code(PARSER, char *code, int size)
{
	int _n = 0;
	int nest = 0;
	char buffer[64];
	enum {CODE,COMMENTBLOCK,COMMENTLINE,STRING,CHAR} state=CODE;
	while (*_p!='\0')
	{
		char c1 = _p[0];
		char c2 = _p[1];
		if (c1=='\n')
			linenum++;
		if (size==0)
		{
			output_error_raw("%s(%d): insufficient buffer space to load code", filename,linenum);
			return 0;
		}
		switch(state) {
		case CODE:
			if (c1==';' && nest==0)
			{
					code[_n]='\0';
					return _n;
			}
			else if (c1=='{')
			{
				nest++;
				COPY(code);
			}
			else if (c1=='}')
			{
				if (nest>0)
				{
					nest--;
					COPY(code);
				}
				else
				{
					output_error_raw("%s(%d): unmatched }", filename,linenum);
					return 0;
				}
			}
			else if (c1=='/' && c2=='*')
				state = COMMENTBLOCK;
			else if (c1=='/' && c2=='/')
				state = COMMENTLINE;
			else if (c1=='"')
			{
				state = STRING;
				COPY(code);
			}
			else if (c1=='\'')
			{
				state = CHAR;
				COPY(code);
			}
			else
				COPY(code);
			break;
		case COMMENTBLOCK:
			if (c1=='*' && c2=='/')
			{
				if (!global_debug_output && global_getvar("noglmrefs",buffer,63)==NULL)
					sprintf(code+strlen(code),"#line %d \"%s\"\n", linenum,forward_slashes(filename));
				state = CODE;
			}
			break;
		case COMMENTLINE:
			if (c1=='\n')
				state = CODE;
			break;
		case STRING:
			if (c1=='"')
				state = CODE;
			else if (c1=='\n')
			{
				output_error_raw("%s(%d): unterminated string constant", filename,linenum);
				return 0;
			}
			COPY(code);
			break;
		case CHAR:
			if (c1=='\'')
				state = CODE;
			else if (c1=='\n')
			{
				output_error_raw("%s(%d): unterminated char constant", filename,linenum);
				return 0;
			}
			COPY(code);
			break;
		default:
			COPY(code);
			break;
		}
	}
	output_error_raw("%s(%d): unterminated code block", filename,linenum);
	return 0;
}

static int class_intrinsic_function(PARSER, CLASS *oclass, int64 *functions, char *code, int size)
{
	char *fname = NULL;
	char *ftype = NULL;
	char arglist[1024];
	char source[65536];
	int startline;
	START;
	if WHITE ACCEPT;
	if (LITERAL("intrinsic") && WHITE && TERM(class_intrinsic_function_name(HERE,oclass,functions,&ftype,&fname)) && (WHITE,TERM(argument_list(HERE,arglist,sizeof(arglist)))) && (startline=linenum,WHITE,TERM(source_code(HERE,source,sizeof(source)))) && (WHITE,LITERAL(";")))
	{
		if (oclass->module==NULL)
		{
			mark_linex(filename,startline);
			append_code("\t%s %s (%s) { OBJECT*my=((OBJECT*)this)-1;",ftype,fname,arglist);
			append_code("\n\ttry %s ",source);
			append_code("catch (char *msg) {callback->output_error(\"%%s[%%s:%%d] exception - %%s\",my->name?my->name:\"(unnamed)\",my->oclass->name,my->id,msg); return 0;} ");
			append_code("catch (const char *msg) {callback->output_error(\"%%s[%%s:%%d] exception - %%s\",my->name?my->name:\"(unnamed)\",my->oclass->name,my->id,msg); return 0;} ");
			append_code("catch (...) {callback->output_error(\"%%s[%%s:%%d] unhandled exception\",my->name?my->name:\"(unnamed)\",my->oclass->name,my->id); return 0;} ");
			append_code("callback->output_error(\"%s::%s(%s) not all paths return a value\"); return 0;}\n",oclass->name,fname,arglist);
			append_code("/*RESETLINE*/\n");
			ACCEPT;
		}
		else
		{
			output_error_raw("%s(%d): intrinsic functions not permitted in static classes", filename, linenum);
			REJECT;
		}
	}
	else
		REJECT;
	DONE;
}

static int class_export_function(PARSER, CLASS *oclass, char *fname, int fsize, char *arglist, int asize, char *code, int csize)
{
	int startline;
	char buffer[64];
	START;
	if WHITE ACCEPT;
	if (LITERAL("export") 
		&& (WHITE,TERM(name(HERE,fname,fsize)))
		&& (WHITE,TERM(argument_list(HERE,arglist,asize))) 
		&& (startline=linenum,WHITE,TERM(source_code(HERE,code,csize))) && (WHITE,LITERAL(";")))
	{
		if (oclass->module==NULL)
		{
			mark_linex(filename,startline);
			append_code("\tstatic int64 %s (%s) %s;\n/*RESETLINE*/\n",fname,arglist,code);

			if (global_getvar("noglmrefs",buffer,63)==NULL)
				append_init("#line %d \"%s\"\n"
					"\tif ((*(callback->function.define))(oclass,\"%s\",(FUNCTIONADDR)&%s::%s)==NULL) return 0;\n"
					"/*RESETLINE*/\n", startline, forward_slashes(filename),
					fname,oclass->name,fname);

			ACCEPT;
		}
		else
		{
			output_error_raw("%s(%d): export functions not permitted in static classes", filename, linenum);
			REJECT;
		}
	}
	else
		REJECT;
	DONE;
}

static int class_explicit_declaration(PARSER, char *type, int size)//, bool *is_static)
{
	START;
	if WHITE ACCEPT;
	if LITERAL("private")
	{
		strcpy(type,"private");
		ACCEPT;
	}
	else if LITERAL("protected")
	{
		strcpy(type,"protected");
		ACCEPT;
	}
	else if LITERAL("public")
	{
		strcpy(type,"public");
		ACCEPT;
	}
	else if LITERAL("static")
	{
		strcpy(type,"static");
		ACCEPT;
	}
	else 
		REJECT;
	WHITE;
/*	if LITERAL("static")
	{
		//strcpy(type,"static");
		*is_static = true;
	} else {
		*is_static = false;
	}
*/
	DONE;
}

static int class_explicit_definition(PARSER, CLASS *oclass)
{
	int startline;
	char type[64];
	char code[4096];
//	bool is_static;
	START;
	if WHITE ACCEPT;
	if (TERM(class_explicit_declaration(HERE,type,sizeof(type)/*,&is_static*/)))
	{
		if (oclass->module==NULL)
		{
			startline=linenum;
			if WHITE ACCEPT;
			if TERM(source_code(HERE,code,sizeof(code)))
			{
				if WHITE ACCEPT;
				if LITERAL(";")
				{
					mark_linex(filename,startline);
					append_code("\t%s: %s;\n",type,code);
					append_code("/*RESETLINE*/\n");
					ACCEPT;
				}
				else
				{
					output_error_raw("%s(%d): missing ; after code block",filename, linenum);
					REJECT;
				}
			}
			else 
			{
				output_error_raw("%s(%d): syntax error in code block",filename, linenum);
				REJECT;
			}
		}
		else
		{
			output_error_raw("%s(%d): explicit definitions not permitted in static classes", filename, linenum);
			REJECT;
		}
	}
	else
		REJECT;
	DONE;
}

static int class_external_function(PARSER, CLASS *oclass, CLASS **eclass,char *fname, int fsize)
{
	CLASSNAME classname;
	START;
	if (LITERAL("function") && WHITE && TERM(name(HERE,classname,sizeof(classname))) && LITERAL("::") && TERM(name(HERE,fname,fsize)) && WHITE,LITERAL(";"))
	{
		if (oclass->module==NULL)
		{
			CLASS *oclass = class_get_class_from_classname(classname);
			if (oclass==NULL) 
			{
				output_error_raw("%s(%d): class '%s' does not exist", filename, linenum, classname);
				REJECT;
			}
			else
			{
				if (class_get_function(classname,fname))
				{
					*eclass = oclass;
					ACCEPT;
				}
				else
				{
					output_error_raw("%s(%d): class '%s' does not define function '%s'", filename, linenum, classname, fname);
					REJECT;
				}
			}
		}
		else
		{
			output_error_raw("%s(%d): external functions not permitted in static classes", filename, linenum);
			REJECT;
		}
	}
	else
		REJECT;
	DONE;
}

static int class_properties(PARSER, CLASS *oclass, int64 *functions, char *initcode, int initsize)
{
	static char code[65536];
	char arglist[1024];
	char fname[64];
	char buffer[64];
	CLASS *eclass;
	PROPERTYTYPE type;
	PROPERTYNAME propname;
	KEYWORD *keys = NULL;
	UNIT *pUnit=NULL;
	START;
	if WHITE ACCEPT;
	if TERM(class_intrinsic_function(HERE,oclass,functions,code,sizeof(code)))
	{
		ACCEPT;
	}
	else if TERM(class_external_function(HERE,oclass,&eclass,fname,sizeof(fname)))
	{
		append_global("FUNCTIONADDR %s::%s = NULL;\n",oclass->name,fname);
		if (global_getvar("noglmrefs",buffer,63)==NULL)
			append_init("#line %d \"%s\"\n\tif ((%s::%s=gl_get_function(\"%s\",\"%s\"))==NULL) throw \"%s::%s not defined\";\n", 
				linenum, forward_slashes(filename), oclass->name, fname, 
				eclass->name, fname, eclass->name, fname);
		append_code("\tstatic FUNCTIONADDR %s;\n",fname);
		ACCEPT;
	}
	else if TERM(class_explicit_definition(HERE, oclass))
	{
		ACCEPT;
	}
	else if TERM(class_export_function(HERE, oclass,fname,sizeof(fname),arglist,sizeof(arglist),code,sizeof(code)))
	{
		*functions |= FN_EXPORT;
		ACCEPT;
	}
	else if (TERM(property_type(HERE,&type,&keys)) && (WHITE,(TERM(nameunit(HERE,propname,sizeof(propname),&pUnit))||TERM(name(HERE,propname,sizeof(propname))))) && (WHITE,LITERAL(";")) )
	{
		PROPERTY *prop = class_find_property(oclass,propname);
		if (prop==NULL)
		{
			if (type==PT_void)
			{
				output_error_raw("%s(%d): property type %s is not recognized", filename, linenum, type);
				REJECT;
			}
			else
			{
				if (pUnit != NULL)
				{
					if (type==PT_double || type==PT_complex || type==PT_random)
						prop = class_add_extended_property(oclass,propname,type,pUnit->name);
					else
					{
						output_error_raw("%s(%d): units not permitted for type %s", filename, linenum, class_get_property_typename(type));
						REJECT;
					}
				}
				else if (keys!=NULL)
				{
					if (type==PT_enumeration || type==PT_set)
					{
						prop = class_add_extended_property(oclass,propname,type,NULL);
						prop->keywords = keys;
					}
					else
					{
						output_error_raw("%s(%d): keys not permitted for type %s", filename, linenum, class_get_property_typename(prop->ptype));
						REJECT;
					}
				}
				else
					prop = class_add_extended_property(oclass,propname,type,NULL);
				if (oclass->module==NULL)
				{
					mark_line();
					if (keys!=NULL)
					{
						KEYWORD *key;
						for (key=prop->keywords; key!=NULL; key=key->next)
							append_code("#define %s (0x%x)\n", key->name, key->value);
					}
					append_code("\t%s %s;\n", class_get_property_typename(prop->ptype), prop->name);
					append_code("/*RESETLINE*/\n");
				}
			}
		}
		else if (prop->ptype!=type)
		{
			output_error_raw("%s(%d): property %s is defined in class %s as type %s", filename, linenum, propname, oclass->name, class_get_property_typename(prop->ptype));
			REJECT;
		}
		ACCEPT;
	}
	else if LITERAL("}") {/* don't accept yet */ DONE;}
	else { syntax_error(HERE); REJECT; }
	/* may be repeated */
	if TERM(class_properties(HERE,oclass,functions,initcode,initsize)) ACCEPT;
	DONE;
}

static int class_block(PARSER)
{
	CLASSNAME classname;
	CLASS *oclass;
	int startline;
	int64 functions = 0;
	char initcode[65536]="";
	char parent[64];
	enum {NONE, PRIVATE, PROTECTED, PUBLIC, EXTERNAL} inherit = NONE;
	START;
	if WHITE ACCEPT;
	if (LITERAL("class") && WHITE) /* enforced whitespace */
	{
		startline = linenum;
		if TERM(name(HERE,classname,sizeof(classname)))
		{
			if (WHITE,LITERAL(":"))
			{
				if WHITE ACCEPT;
				if (LITERAL("public") && WHITE && TERM(name(HERE,parent,sizeof(parent))) )
				{
					inherit = PUBLIC;
					ACCEPT;
				}
				else if (LITERAL("protected") && WHITE && TERM(name(HERE,parent,sizeof(parent))) )
				{
					inherit = PROTECTED;
					ACCEPT;
				}
				else if (LITERAL("private") && WHITE && TERM(name(HERE,parent,sizeof(parent))) )
				{
					inherit = PRIVATE;
					ACCEPT;
				}
				else
				{
					output_error_raw("%s(%d): missing inheritance qualifier", filename, linenum);
					REJECT;
					DONE;
				}
				if (class_get_class_from_classname(parent)==NULL)
				{
					output_error_raw("%s(%d): class %s inherits from undefined class %s", filename, linenum, classname, parent);
					REJECT;
					DONE;
				}
			}
			if WHITE ACCEPT;
			if LITERAL("{")
			{
				oclass = class_get_class_from_classname(classname);
				if (oclass==NULL)
				{
					oclass = class_register(NULL,classname,0,0x00);
					mark_line();
					switch (inherit) {
					case NONE:
						append_code("class %s {\npublic:\n\t%s(MODULE*mod) {};\n", oclass->name, oclass->name);
						break;
					case PRIVATE:
						append_code("class %s : private %s {\npublic:\n\t%s(MODULE*mod) : %s(mod) {};\n", oclass->name, parent, oclass->name, parent);
						oclass->parent = class_get_class_from_classname(parent);
						break;
					case PROTECTED:
						append_code("class %s : protected %s {\npublic:\n\t%s(MODULE*mod) : %s(mod) {};\n", oclass->name, parent, oclass->name, parent);
						oclass->parent = class_get_class_from_classname(parent);
						break;
					case PUBLIC:
						append_code("class %s : public %s {\npublic:\n\t%s(MODULE*mod) : %s(mod) {};\n", oclass->name, parent, oclass->name, parent);
						oclass->parent = class_get_class_from_classname(parent);
						break;
					default:
						output_error("class_block inherit status is invalid (inherit=%d)", inherit);
						REJECT;
						DONE;
						break;
					}
					mark_line();
				}
				ACCEPT;
			}
			else
			{
				output_error_raw("%s(%d): expected class %s block opening {",filename,linenum,classname);
				REJECT;
			}
		}
		else
		{
			output_error_raw("%s(%d): expected class name",filename,linenum);
			REJECT;
		}
		if (TERM(class_properties(HERE,oclass,&functions,initcode,sizeof(initcode)))) ACCEPT;
		if WHITE ACCEPT;
		if LITERAL("}")
		{
			if ( oclass->module==NULL && functions!=0 )
			{
				append_code("};\n");
#define ENTERING(OBJ,X) if (strstr(global_trace,#X)!=NULL) append_code("trace(\"call %s::%s\",("#OBJ"));",oclass->name,#X)
#define EXITING(OBJ,X) if (strstr(global_trace,#X)!=NULL) append_code("trace(\"exit %s::%s\",("#OBJ"));",oclass->name,#X)

				append_code("/*RESETLINE*/\n");
				append_code("/*RESETLINE*/\n");
				append_code("extern \"C\" int64 create_%s(OBJECT **obj, OBJECT *parent)\n{\n",oclass->name);
				append_code(
						"\tif ((*obj=gl_create_object(myclass))==NULL)\n\t\treturn 0;\n"
						"\tgl_set_parent(*obj,parent);\n", oclass->name,oclass->name);
					if (functions&FN_CREATE) 
					{
						ENTERING(*obj,create);
						append_code("\tint64 ret = ((%s*)((*obj)+1))->create(parent);\n",oclass->name);
						EXITING(*obj,create);
						append_code("\treturn ret;\n}\n");
					}
					else
						append_code("\treturn 1;\n}\n");
				if (functions&FN_INIT) {
					append_code("/*RESETLINE*/\n");
					append_code("extern \"C\" int64 init_%s(OBJECT *obj, OBJECT *parent)\n{\n",oclass->name);
					ENTERING(*obj,init);
					append_code("\tint64 ret = ((%s*)(obj+1))->init(parent);\n",oclass->name);
					EXITING(*obj,init);
					append_code("\treturn ret;\n}\n");
				}
				if (functions&FN_PRECOMMIT) {
					append_code("/*RESETLINE*/\n");
					append_code("extern \"C\" int64 precommit_%s(OBJECT *obj, TIMESTAMP t1)\n{\n",oclass->name);
					ENTERING(*obj,precommit);
					append_code("\tint64 ret = ((%s*)(obj+1))->precommit(t1);\n",oclass->name);
					EXITING(*obj,precommit);
					append_code("\treturn ret;\n}\n");
				}
				if (functions&FN_SYNC || functions&FN_PRESYNC || functions&FN_POSTSYNC) {
					append_code("/*RESETLINE*/\n");
					append_code("extern \"C\" int64 sync_%s(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)\n{\n",oclass->name);
					append_code("\tint64 t2 = TS_NEVER;\n\tswitch (pass) {\n");
					if (functions&FN_PRESYNC)
					{
						append_code("\tcase PC_PRETOPDOWN:\n");
						ENTERING(obj,presync);
						append_code("\t\tt2=((%s*)(obj+1))->presync(obj->clock,t1);\n",oclass->name);
						EXITING(obj,presync);
						if ((functions&(FN_SYNC|FN_POSTSYNC))==0)
							append_code("\t\tobj->clock = t1;\n");
						append_code("\t\tbreak;\n");
					}
					if (functions&FN_SYNC)
					{
						append_code("\tcase PC_BOTTOMUP:\n");
						ENTERING(obj,sync);
						append_code("\t\tt2=((%s*)(obj+1))->sync(obj->clock,t1);\n",oclass->name);
						EXITING(obj,sync);
						if ((functions&FN_POSTSYNC)==0)
							append_code("\t\tobj->clock = t1;\n");
						append_code("\t\tbreak;\n");
					}
					if (functions&FN_POSTSYNC)
					{
						append_code("\tcase PC_POSTTOPDOWN:\n");
						ENTERING(obj,postsync);
						append_code("\t\tt2=((%s*)(obj+1))->postsync(obj->clock,t1);\n",oclass->name);
						EXITING(obj,postsync);
						append_code("\t\tobj->clock = t1;\n");
						append_code("\t\tbreak;\n");
					}
					append_code("\tdefault:\n\t\tbreak;\n\t}\n\treturn t2;\n}\n");
				}
				if (functions&FN_PLC) {
					append_code("/*RESETLINE*/\n");
					append_code("extern \"C\" int64 plc_%s(OBJECT *obj, TIMESTAMP t1)\n{\n",oclass->name);
					ENTERING(obj,plc);
					append_code("\tint64 t2 = ((%s*)(obj+1))->plc(obj->clock,t1);\n",oclass->name);
					EXITING(obj,plc);
					append_code("\treturn t2;\n}\n");
				}
				if (functions&FN_COMMIT) {
					append_code("/*RESETLINE*/\n");
					append_code("extern \"C\" TIMESTAMP commit_%s(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2)\n{\n",oclass->name);
					ENTERING(obj,commit);
					append_code("\tTIMESTAMP ret = ((%s*)(obj+1))->commit(t1, t2);\n",oclass->name);
					EXITING(obj,commit);
					append_code("\treturn ret;\n}\n");
				}
				if (functions&FN_ISA) {
					append_code("/*RESETLINE*/\n");
					append_code("extern \"C\" int64 isa_%s(OBJECT *obj, char *type)\n{\n",oclass->name);
					ENTERING(obj,isa);
					append_code("\tint64 ret = ((%s*)(obj+1))->isa(type);\n",oclass->name);
					EXITING(obj,isa);
					append_code("\treturn ret;\n}\n");
				}
				if (functions&FN_NOTIFY) {
					append_code("/*RESETLINE*/\n");
					append_code("extern \"C\" int64 notify_%s(OBJECT *obj, NOTIFYMODULE msg)\n{\n",oclass->name);
					ENTERING(obj,notify);
					append_code("\tint ret64 = ((%s*)(obj+1))->isa(type);\n",oclass->name);
					EXITING(obj,notify);
					append_code("\treturn ret;\n}\n");
				}
				if (functions&FN_RECALC) {
					append_code("/*RESETLINE*/\n");
					append_code("extern \"C\" int64 recalc_%s(OBJECT *obj)\n{\n",oclass->name);
					ENTERING(obj,recalc);
					append_code("\tint ret64 = ((%s*)(obj+1))->recalc();\n",oclass->name);
					EXITING(obj,recalc);
					append_code("\treturn ret;\n}\n");
				}
				if (functions&FN_FINALIZE) {
					append_code("/*RESETLINE*/\n");
					append_code("extern \"C\" int64 finalize_%s(OBJECT *obj)\n{\n",oclass->name);
					ENTERING(*obj,create);
					append_code("\tint64 ret = ((%s*)(obj+1))->finalize();\n",oclass->name);
					EXITING(*obj,create);
					append_code("\treturn ret;\n}\n");
				}

				/* TODO add other intrinsics (notify, recalc, isa) */
				if (!compile_code(oclass,functions)) REJECT;
			} else if ( functions!=0 ) { // if module != NULL
				if(code_used){
					output_error_raw("%s(%d): intrinsic functions found for compiled class", filename, linenum);
					REJECT;
				}
			}				
			ACCEPT;
		}
		else
		{
			output_error_raw("%s(%d): expected closing } after class block", filename, linenum);
			REJECT;
		}
	}
	else REJECT;
	DONE;
}

int set_flags(OBJECT *obj, char *propval)
{
	extern KEYWORD oflags[];
	if (convert_to_set(propval,&(obj->flags),object_flag_property())<=0)
	{
		output_error_raw("%s(%d): flags of %s:%d %s could not be set to %s", filename, linenum, obj->oclass->name, obj->id, obj->name, propval);
		return 0;
	};
	return 1;
}

int is_int(PROPERTYTYPE pt){
	if(pt == PT_int16 || pt == PT_int32 || pt == PT_int64){
		return (int)pt;
	} else {
		return 0;
	}
}

static int schedule_ref(PARSER, SCHEDULE **sch)
{
	char name[64];
	START;
	if WHITE ACCEPT;
	if (TERM(dashed_name(HERE,name,sizeof(name))))
	{
		ACCEPT;
		if (((*sch)=schedule_find_byname(name))==NULL)
			REJECT;
	}
	else
		REJECT;
	DONE;
}
static int property_ref(PARSER, TRANSFORMSOURCE *xstype, void **ref, OBJECT *from)
{
	FULLNAME oname;
	PROPERTYNAME pname;
	START;
	if WHITE ACCEPT;
	if (TERM(name(HERE,oname,sizeof(oname))) && LITERAL(".") && TERM(dotted_name(HERE,pname,sizeof(pname))))
	{
		OBJECT *obj = (strcmp(oname,"this")==0 ? from : object_find_name(oname));

		// object isn't defined yet
		if (obj==NULL)
		{
			// add to unresolved list
			char id[1024];
			sprintf(id,"%s.%s",oname,pname);
			*ref = (void*)add_unresolved(from,PT_double,NULL,from->oclass,id,filename,linenum,UR_TRANSFORM);
			ACCEPT;
		}
		else 
		{
			PROPERTY *prop = object_get_property(obj,pname,NULL);
			if (prop==NULL)
			{
				output_error_raw("%s(%d): property '%s' of object '%s' not found", filename, linenum, oname,pname);
				REJECT;
			}
			else if (prop->ptype==PT_double)
			{
				*ref = (void*)object_get_addr(obj,pname); 
				*xstype = XS_DOUBLE;
				ACCEPT;
			}
			else if (prop->ptype==PT_complex)
			{
				// TODO support R,I parts
				*ref = (void*)object_get_addr(obj,pname); // get R part only
				*xstype = XS_COMPLEX;
				ACCEPT;
			}
			else if (prop->ptype==PT_loadshape)
			{
				loadshape *ls = (void*)object_get_addr(obj,pname);
				*ref = &(ls->load);
				*xstype = XS_LOADSHAPE;
				ACCEPT;
			}
			else if (prop->ptype==PT_enduse)
			{
				enduse *eu = (void*)object_get_addr(obj,pname);
				*ref = &(eu->total.r);
				*xstype = XS_ENDUSE;
			}
			else
			{
				output_error_raw("%s(%d): transform '%s.%s' does not reference a double or a double container like a loadshape", filename, linenum, oname,pname);
				REJECT;
			}
		}
	}
	else
	{	REJECT;	}
	DONE;
}

static int transform_source(PARSER, TRANSFORMSOURCE *xstype, void **source, OBJECT *from)
{
	SCHEDULE *sch;
	START;
	if WHITE ACCEPT;
	if (TERM(schedule_ref(HERE,&sch)))
	{
		*source = (void*)&(sch->value);
		*xstype = XS_SCHEDULE;
		ACCEPT;
	}
	else if (TERM(property_ref(HERE,xstype,source,from)))
	{	ACCEPT; }
	else
	{	REJECT; }
	DONE;
}

static int external_transform(PARSER, TRANSFORMSOURCE *xstype, char *sources, size_t srcsize, char *functionname, size_t namesize, OBJECT *from)
{
	char fncname[1024];
	char varlist[4096];
	START;
	if ( TERM(name(HERE,fncname,sizeof(fncname))) && WHITE,LITERAL("(") && WHITE,TERM(variable_list(HERE,varlist,sizeof(varlist))) && LITERAL(")") )
	{
		if ( strlen(fncname)<namesize && strlen(varlist)<srcsize )
		{
			strcpy(functionname,fncname);
			strcpy(sources,varlist);
			ACCEPT;
			DONE
		}
	}
	REJECT;
	DONE;
}
static int linear_transform(PARSER, TRANSFORMSOURCE *xstype, void **source, double *scale, double *bias, OBJECT *from)
{
	START;
	if WHITE ACCEPT;
	/* scale * schedule_name [+ bias]  */
	if (TERM(functional(HERE,scale)) && (WHITE,LITERAL("*")) && (WHITE,TERM(transform_source(HERE, xstype, source, from))))
	{	
		if ((WHITE,LITERAL("+")) && (WHITE,TERM(functional(HERE,bias)))) { ACCEPT; }
		else { *bias = 0;	ACCEPT;}
		DONE;
	}
	OR
	/* scale * schedule_name [- bias]  */
	if (TERM(functional(HERE,scale)) &&( WHITE,LITERAL("*")) && (WHITE,TERM(transform_source(HERE,xstype, source,from))))
	{
		if ((WHITE,LITERAL("-")) && (WHITE,TERM(functional(HERE,bias)))) { *bias *= -1; ACCEPT; }
		else { *bias = 0;	ACCEPT;}
		DONE;
	}
	OR
	/* schedule_name [* scale] [+ bias]  */
	if (TERM(transform_source(HERE,xstype,source,from)))
	{
		if ((WHITE,LITERAL("*")) && (WHITE,TERM(functional(HERE,scale)))) { ACCEPT; }
		else { ACCEPT; *scale = 1;}
		if ((WHITE,LITERAL("+")) && (WHITE,TERM(functional(HERE,bias)))) { ACCEPT; DONE; }
	 	OR if ((WHITE,LITERAL("-")) && (WHITE,TERM(functional(HERE,bias)))) { *bias *= -1; ACCEPT; DONE}
		else { *bias = 0;	ACCEPT;}
		DONE;
	}
	OR
	/* bias + scale * schedule_name  */
	if (TERM(functional(HERE,bias)) && (WHITE,LITERAL("+")) && (WHITE,TERM(functional(HERE,scale))) && (WHITE,LITERAL("*")) && (WHITE,TERM(transform_source(HERE,xstype, source,from))))
	{
		ACCEPT;
		DONE;
	}
	OR
	/* bias - scale * schedule_name  */
	if (TERM(functional(HERE,bias)) && (WHITE,LITERAL("-")) && (WHITE,TERM(functional(HERE,scale))) && (WHITE,LITERAL("*")) && (WHITE,TERM(transform_source(HERE,xstype, source,from))))
	{
		*scale *= -1;
		ACCEPT;
		DONE;
	}
	OR
	/* bias + schedule_name [* scale] */
	if (TERM(functional(HERE,bias)) && (WHITE,LITERAL("+")) && (WHITE,TERM(transform_source(HERE,xstype, source,from))))
	{
		if (WHITE,LITERAL("*") && WHITE,TERM(functional(HERE,scale))) { ACCEPT; }
		else { ACCEPT; *scale = 1;}
		DONE;
	}
	OR
	/* bias - schedule_name [* scale] */
	if (TERM(functional(HERE,bias)) && WHITE,LITERAL("-") && WHITE,TERM(transform_source(HERE,xstype, source,from)))
	{
		if ((WHITE,LITERAL("*")) && (WHITE,TERM(functional(HERE,scale)))) { ACCEPT; *scale *= -1; }
		else { ACCEPT; *scale = 1;}
		DONE;
	}
	REJECT;
	DONE;
}

static int object_block(PARSER, OBJECT *parent, OBJECT **obj);
static int object_properties(PARSER, CLASS *oclass, OBJECT *obj)
{
	PROPERTYNAME propname;
	char1024 propval;
	double dval;
	complex cval;
	void *source=NULL;
	TRANSFORMSOURCE xstype = XS_UNKNOWN;
	char transformname[1024];
	char sources[4096];
	double scale=1,bias=0;
	UNIT *unit=NULL;
	OBJECT *subobj=NULL;
	START;
	if WHITE ACCEPT;
	if TERM(line_spec(HERE)) {ACCEPT;}
	if WHITE ACCEPT;
	if TERM(object_block(HERE,obj,&subobj)) 
	{		
		if (WHITE,LITERAL(";"))
		{	ACCEPT;}
		else
		{
			output_error_raw("%s(%d): missing ; at end of nested object block", filename, linenum,propname);
			REJECT;
		}
			
	}
	else if (TERM(dotted_name(HERE,propname,sizeof(propname))) && WHITE)
	{
		PROPERTY *prop = class_find_property(oclass,propname);
		OBJECT *subobj=NULL;
		current_object = obj; /* object context */
		current_module = obj->oclass->module; /* module context */
		if (prop!=NULL && prop->ptype==PT_object && TERM(object_block(HERE,NULL,&subobj)))
		{
			char objname[64];
			if (subobj->name) strcpy(objname,subobj->name); else sprintf(objname,"%s:%d", subobj->oclass->name,subobj->id);
			if (object_set_value_by_name(obj,propname,objname))
				ACCEPT
			else
			{
				output_error_raw("%s(%d): unable to link subobject to property '%s'", filename, linenum,propname);
				REJECT;
			}
		}
		else if (prop!=NULL && prop->ptype==PT_complex && TERM(complex_unit(HERE,&cval,&unit)))
		{
			if (unit!=NULL && prop->unit!=NULL && strcmp((char *)unit, "") != 0 && unit_convert_complex(unit,prop->unit,&cval)==0)
			{
				output_error_raw("%s(%d): units of value are incompatible with units of property, cannot convert from %s to %s", filename, linenum, unit->name,prop->unit->name);
				REJECT;
			}
			else if (object_set_complex_by_name(obj,propname,cval)==0)
			{
				output_error_raw("%s(%d): property %s of %s %s could not be set to '%g%+gi'", filename, linenum, propname, format_object(obj), cval.r, cval.i);
				REJECT;
			}
			else
				ACCEPT;
		}
		else if (prop!=NULL && prop->ptype==PT_double && TERM(expression(HERE, &dval, &unit, obj)))
		{
			if (unit!=NULL && prop->unit!=NULL && strcmp((char *)unit, "") != 0 && unit_convert_ex(unit,prop->unit,&dval)==0)
			{
				output_error_raw("%s(%d): units of value are incompatible with units of property, cannot convert from %s to %s", filename, linenum, unit->name,prop->unit->name);
				REJECT;
			}
			else if (object_set_double_by_name(obj,propname,dval)==0)
			{
				output_error_raw("%s(%d): property %s of %s %s could not be set to '%g'", filename, linenum, propname, format_object(obj), dval);
				REJECT;
			}
			else
				ACCEPT;
		} 
		else if (prop!=NULL && prop->ptype==PT_double && TERM(functional_unit(HERE,&dval,&unit)))
		{
			if (unit!=NULL && prop->unit!=NULL && strcmp((char *)unit, "") != 0 && unit_convert_ex(unit,prop->unit,&dval)==0)
			{
				output_error_raw("%s(%d): units of value are incompatible with units of property, cannot convert from %s to %s", filename, linenum, unit->name,prop->unit->name);
				REJECT;
			}
			else if (object_set_double_by_name(obj,propname,dval)==0)
			{
				output_error_raw("%s(%d): property %s of %s %s could not be set to '%g'", filename, linenum, propname, format_object(obj), dval);
				REJECT;
			}
			else
				ACCEPT;
		}
		else if(prop != NULL && is_int(prop->ptype) && TERM(functional_unit(HERE, &dval, &unit))){
			int64 ival = 0;
			int16 ival16 = 0;
			int32 ival32 = 0;
			int64 ival64 = 0;
			int rv = 0;

			if(unit != NULL && prop->unit != NULL && strcmp((char *)(unit), "") != 0 && unit_convert_ex(unit, prop->unit, &dval) == 0){
				output_error_raw("%s(%d): units of value are incompatible with units of property, cannot convert from %s to %s", filename, linenum, unit->name,prop->unit->name);
				REJECT;
			} else {
				switch(prop->ptype){
					case PT_int16:
						ival16 = (int16)dval;
						ival = rv = object_set_int16_by_name(obj, propname, ival16);
						break;
					case PT_int32:
						ival = ival32 = (int32)dval;
						rv = object_set_int32_by_name(obj, propname, ival32);
						break;
					case PT_int64:
						ival = ival64 = (int64)dval;
						rv = object_set_int64_by_name(obj, propname, ival64);
						break;
					default:
						output_error("function_int operating on a non-integer (we shouldn't be here)");
						REJECT;
				} /* end switch */
				if(rv == 0){
					output_error_raw("%s(%d): property %s of %s %s could not be set to '%g'", filename, linenum, propname, format_object(obj), ival);
					REJECT;
				} else {
					ACCEPT;
				}
#if 0
			if (object_set_double_by_name(obj,propname,dval)==0)
			{
				output_message("%s(%d): property %s of %s %s could not be set to '%g'", filename, linenum, propname, format_object(obj), dval);
				REJECT;
			} else {
				ACCEPT;
			}
#endif
			} /* end unit_convert_ex else */
		}
		else if (prop!=NULL 
			&& ( ( prop->ptype>=PT_double && prop->ptype<=PT_int64 ) || ( prop->ptype>=PT_bool && prop->ptype<=PT_timestamp ) || ( prop->ptype>=PT_float && prop->ptype<=PT_enduse ) )
			&& TERM(linear_transform(HERE, &xstype, &source,&scale,&bias,obj)))
		{
			void *target = (void*)((char*)(obj+1) + (int64)prop->addr);

			/* add the transform list */
			if (!transform_add_linear(xstype,source,target,scale,bias,obj,prop,(xstype == XS_SCHEDULE ? source : 0)))
			{
				output_error_raw("%s(%d): schedule transform could not be created - %s", filename, linenum, errno?strerror(errno):"(no details)");
				REJECT;
			}
			else if ( source!=NULL )
			{
				/* a transform is unresolved */
				if (first_unresolved==source)

					/* source was the unresolved entry, for now it will be the transform itself */
					first_unresolved->ref = (void*)transform_getnext(NULL);

				ACCEPT;
			}
		}
		else if (prop!=NULL && prop->ptype==PT_double && TERM(external_transform(HERE, &xstype, sources, sizeof(sources), transformname, sizeof(transformname), obj)))
		{
			// TODO handle more than one source
			char sobj[64], sprop[64];
			int n = sscanf(sources,"%[^.].%[^,]",sobj,sprop);
			OBJECT *source_obj;
			PROPERTY *source_prop;

			/* get source object */
			source_obj = (n==1||strcmp(sobj,"this")==0) ? obj : object_find_name(sobj);
			if ( !source_obj )
			{
				output_error_raw("%s(%d): transform source object '%s' not found", filename, linenum, n==1?"this":sobj);
				REJECT;
				DONE;
			}

			/* get source property */
			source_prop = object_get_property(source_obj, n==1?sobj:sprop,NULL);
			if ( !source_prop )
			{
				output_error_raw("%s(%d): transform source property '%s' of object '%s' not found", filename, linenum, n==1?sobj:sprop, n==1?"this":sobj);
				REJECT;
				DONE;
			}

			/* add to external transform list */
			if ( !transform_add_external(obj,prop,transformname,source_obj,source_prop) )
			{
				output_error_raw("%s(%d): schedule transform could not be created - %s", filename, linenum, errno?strerror(errno):"(no details)");
				REJECT;
				DONE;
			}
			else if ( source!=NULL )
			{
				/* a transform is unresolved */
				if (first_unresolved==source)

					/* source was the unresolved entry, for now it will be the transform itself */
					first_unresolved->ref = (void*)transform_getnext(NULL);

				ACCEPT;
			}
		}
		else if TERM(alternate_value(HERE,propval,sizeof(propval)))
		{
			if (prop==NULL)
			{
				/* check for special properties */
				if (strcmp(propname,"root")==0)
					obj->parent = NULL;
				else if (strcmp(propname,"parent")==0)
				{
					if (add_unresolved(obj,PT_object,(void*)&obj->parent,oclass,propval,filename,linenum,UR_RANKS)==NULL)
					{
						output_error_raw("%s(%d): unable to add unresolved reference to parent %s", filename, linenum, propval);
						REJECT;
					}
					else
						ACCEPT;
				}
				else if (strcmp(propname,"rank")==0)
				{
					if ((obj->rank = atoi(propval))<0)
					{
						output_error_raw("%s(%d): unable to set rank to %s", filename, linenum, propval);
						REJECT;
					}
					else
						ACCEPT;
				}
				else if (strcmp(propname,"clock")==0)
				{
					obj->clock = atoi64(propval); // @todo convert_to_timestamp should be used
					ACCEPT;
				}
				else if (strcmp(propname,"valid_to")==0)
				{
					obj->valid_to = atoi64(propval); // @todo convert_to_timestamp should be used
					ACCEPT;
				}
				else if (strcmp(propname,"schedule_skew")==0)
				{
					obj->schedule_skew = atoi64(propval);
					ACCEPT;
				}
				else if (strcmp(propname,"latitude")==0)
				{
					obj->latitude = load_latitude(propval);
					ACCEPT;
				}
				else if (strcmp(propname,"longitude")==0)
				{
					obj->longitude = load_longitude(propval);
					ACCEPT;
				}
				else if (strcmp(propname,"in")==0)
				{
					obj->in_svc = convert_to_timestamp_delta(propval,&obj->in_svc_micro,&obj->in_svc_double);
					ACCEPT;
				}
				else if (strcmp(propname,"out")==0)
				{
					obj->out_svc = convert_to_timestamp_delta(propval,&obj->out_svc_micro,&obj->out_svc_double);
					ACCEPT;
				}
				else if (strcmp(propname,"name")==0)
				{
					if (object_set_name(obj,propval)==NULL)
					{
						output_error_raw("%s(%d): property name %s could not be used", filename, linenum, propval);
						REJECT;
					}
					else
						ACCEPT;
				}
				else if ( strcmp(propname,"heartbeat")==0 )
				{
					obj->heartbeat = convert_to_timestamp(propval);
					ACCEPT;
				}
				else if (strcmp(propname,"groupid")==0){
					strncpy(obj->groupid, propval, sizeof(obj->groupid));
				}
				else if (strcmp(propname,"flags")==0)
				{
					if(set_flags(obj,propval) == 0)
					{
						REJECT;
					}
					else
						ACCEPT;
				}
				else if (strcmp(propname,"library")==0)
				{
					output_warning("%s(%d): libraries not yet supported", filename, linenum);
					/* TROUBLESHOOT
						An attempt to use the <b>library</b> GLM directive was made.  Library directives
						are not supported yet.
					 */
					ACCEPT;
					DONE;
				}
				else
				{
					output_error_raw("%s(%d): property %s is not defined in class %s", filename, linenum, propname, oclass->name);
					REJECT;
				}
			}
			else if (prop->ptype==PT_object)
			{	void *addr = object_get_addr(obj,propname);
				if (addr==NULL)
				{
					output_error_raw("%s(%d): unable to get %s member %s", filename, linenum, format_object(obj), propname);
					REJECT;
				}
				else
				{
					add_unresolved(obj,PT_object,addr,oclass,propval,filename,linenum,UR_NONE);
					ACCEPT;
				}
			}
			else if (object_set_value_by_name(obj,propname,propval)==0)
			{
				output_error_raw("%s(%d): property %s of %s could not be set to '%s'", filename, linenum, propname, format_object(obj), propval);
				REJECT;
			}
			else
				ACCEPT; // @todo shouldn't this be REJECT?
		}
		if WHITE ACCEPT;
		if LITERAL(";") {ACCEPT;}
		else
		{
			output_error_raw("%s(%d): expected ';' at end of property specification", filename,linenum);
			REJECT;
		}
	}
	else if LITERAL("}") {/* don't accept yet */ DONE;}
	else { syntax_error(HERE); REJECT; }
	/* may be repeated */
	if TERM(object_properties(HERE,oclass,obj)) ACCEPT;
	DONE;
}

static int object_name_id(PARSER,char *classname, int64 *id)
{
	START;
	if WHITE ACCEPT;
	if TERM(dotted_name(HERE,classname,sizeof(CLASSNAME)))
	{
		*id = -1; /* anonymous object */
		if LITERAL(":")
		{
			TERM(integer(HERE,id));
		}
		ACCEPT;
		DONE;
	}
	else if TERM(name(HERE,classname,sizeof(CLASSNAME)))
	{
		*id = -1; /* anonymous object */
		if LITERAL(":")
		{
			TERM(integer(HERE,id));
		}
		ACCEPT;
		DONE;
	}
	else
		REJECT;
}

static int object_name_id_range(PARSER,char *classname, int64 *from, int64 *to)
{
	START;
	if WHITE ACCEPT;
	if (TERM(dotted_name(HERE,classname,sizeof(CLASSNAME))) && LITERAL(":") && TERM(integer(HERE,from)) && LITERAL("..")) ACCEPT
	else if (TERM(name(HERE,classname,sizeof(CLASSNAME))) && LITERAL(":") && TERM(integer(HERE,from)) && LITERAL("..")) ACCEPT
	else REJECT;
	if (TERM(integer(HERE,to))) ACCEPT else
	{
		output_error_raw("%s(%d): expected id range end value", filename, linenum);
		REJECT;
	}
	DONE;
}

static int object_name_id_count(PARSER,char *classname, int64 *count)
{
	START;
	if WHITE ACCEPT;
	if (TERM(dotted_name(HERE,classname,sizeof(CLASSNAME))) && LITERAL(":") && LITERAL("..")) ACCEPT
	else if (TERM(name(HERE,classname,sizeof(CLASSNAME))) && LITERAL(":") && LITERAL("..")) ACCEPT
	else REJECT;
	if (TERM(integer(HERE,count))) ACCEPT else
	{
		output_error_raw("%s(%d): expected id count", filename, linenum);
		REJECT;
	}
	DONE;
}

static int object_block(PARSER, OBJECT *parent, OBJECT **subobj)
{
#define NAMEOBJ  /* DPC: not sure what this does, but it doesn't seem to be harmful */
#ifdef NAMEOBJ
	static OBJECT nameobj;
#endif
	FULLNAME space;
	CLASSNAME classname;
	CLASS *oclass;
	OBJECT *obj=NULL;
	int64 id=-1, id2=-1;
	START;

	// @TODO push context here

	if WHITE ACCEPT;
	if (LITERAL("namespace") && (WHITE,TERM(name(HERE,space,sizeof(space)))) && (WHITE,LITERAL("{")))
	{
		if (!object_open_namespace(space))
		{
			output_error_raw("%s(%d): namespace %s could not be opened", filename, linenum, space);
			REJECT;
		}

		while (TERM(object_block(HERE,parent,subobj))) {LITERAL(";");}
		while (WHITE);
		if (LITERAL("}"))
		{	object_close_namespace();
			ACCEPT;
			DONE;
		}
		else
		{
			output_error_raw("%s(%d): namespace %s missing closing }", filename, linenum, space);
			REJECT;
		}
	}

	if WHITE ACCEPT;
	if (LITERAL("object") && WHITE) ACCEPT else REJECT /* enforced whitespace */

	/* objects should not be started until all deferred schedules are done */
	if ( global_threadcount>1 )
	{
		if ( schedule_createwait()==FAILED )
		{
			output_error_raw("%s(%d): object create cannot proceed when a schedule error persists", filename, linenum);
			REJECT;
		}
	}

	//if WHITE ACCEPT;
	if TERM(object_name_id_range(HERE,classname,&id,&id2))
	{
		oclass = class_get_class_from_classname(classname);
		if (oclass==NULL) 
		{
			output_error_raw("%s(%d): class '%s' is not known", filename, linenum, classname);
			REJECT;
		}
		id2++;
		ACCEPT;
	}
	else if TERM(object_name_id_count(HERE,classname,&id2))
	{
		id=-1; id2--;  /* count down to zero inclusive */
		oclass = class_get_class_from_classname(classname);
		if (oclass==NULL) 
		{
			output_error_raw("%s(%d): class '%s' is not known", filename, linenum, classname);
			REJECT;
		}
		ACCEPT;
	}
	else if TERM(object_name_id(HERE,classname,&id))
	{
		oclass = class_get_class_from_classname(classname);
		if (oclass==NULL)
		{
			output_error_raw("%s(%d): class '%s' is not known", filename, linenum, classname);
			REJECT;
		}
		ACCEPT;
	}
	else
	{
		output_error_raw("%s(%d): expected object id or range", filename, linenum);
		REJECT;
	}
	if WHITE ACCEPT;
	if (LITERAL("{")) ACCEPT else
	{
		output_error_raw("%s(%d): expected object block starting {", filename, linenum);
		REJECT;
	}

	/* id(s) is/are valid */
#ifdef NAMEOBJ
	nameobj.name = classname;
#endif
	if (id2==-1) id2=id+1; /* create singleton */
	BEGIN_REPEAT;
	while (id<id2)
	{
		REPEAT;
		if (oclass->create!=NULL)
		{
#ifdef NAMEOBJ
			obj = &nameobj;
#endif
			if ((*oclass->create)(&obj,parent)==0) 
			{
				output_error_raw("%s(%d): create failed for object %s:%d", filename, linenum, classname, id);
				REJECT;
			}
			else if (obj==NULL
#ifdef NAMEOBJ
				|| obj==&nameobj
#endif
				) 
			{
				output_error_raw("%s(%d): create failed name object %s:%d", filename, linenum, classname, id);
				REJECT;
			}
		}
		else // need to create object here because class has no create function 
		{
			obj = object_create_single(oclass);
			if ( obj==NULL )
			{
				output_error_raw("%s(%d): create failed for object %s:%d", filename, linenum, classname, id);
				REJECT;
			}
			object_set_parent(obj,parent);
		}
		if (id!=-1 && load_set_index(obj,(OBJECTNUM)id)==FAILED)
		{
			output_error_raw("%s(%d): unable to index object id number for %s:%d", filename, linenum, classname, id);
			REJECT;
		}
		else if TERM(object_properties(HERE,oclass,obj))
		{
			ACCEPT;
		} 
		else REJECT;
		if (id==-1) id2--; else id++;
	}
	END_REPEAT;
	if WHITE ACCEPT;
	if LITERAL("}") ACCEPT else
	{
		output_error_raw("%s(%d): expected object block closing }", filename, linenum);
		REJECT;
	}
	if (subobj) *subobj = obj;
	
	// @TODO pop context here

	DONE;
}

static int import(PARSER)
{
	char32 modname;
	char1024 fname;
	START;
	if WHITE ACCEPT;
	if (LITERAL("import") && WHITE) /* enforced whitespace */
	{
		if (TERM(name(HERE,modname,sizeof(modname))) && WHITE) /* enforced whitespace */
		{
			current_object = NULL; /* object context */
			current_module = NULL; /* module context */
			if TERM(alternate_value(HERE,fname,sizeof(fname)))
			{
				if LITERAL(";")
				{
					int result;
					MODULE *module = module_find(modname);
					if (module==NULL)
					{
						output_error_raw("%s(%d): module %s not loaded", filename, linenum, modname);
						REJECT;
					}
					result = module_import(module,fname);
					if (result < 0)
					{
						output_error_raw("%s(%d): %d errors loading importing %s into %s module", filename, linenum, -result, fname, modname);
						REJECT;
					}
					else if (result==0)
					{
						output_error_raw("%s(%d): module %s load of %s failed; %s", filename, linenum, modname, fname, errno?strerror(errno):"(no details)");
						REJECT;
					}
					else
					{
						output_verbose("%d objects loaded to %s from %s", result, modname, fname);
						ACCEPT;
					}
				}
				else
				{
					output_error_raw("%s(%d): expected ; after module %s import from %s statement", filename, linenum, modname, fname);
					REJECT;
				}
			}
			else
			{
				output_error_raw("%s(%d): expected filename after module %s import statement", filename, linenum, modname);
				REJECT;
			}
		}
		else
		{
			output_error_raw("%s(%d): expected module name after import statement", filename, linenum);
			REJECT;
		}
	}
	DONE
}

static int library(PARSER)
{
	START;
	if WHITE ACCEPT;
	if (LITERAL("library") && WHITE) /* enforced whitespace */
	{
		char libname[1024];
		if ( TERM(dotted_name(HERE,libname,sizeof(libname))) && (WHITE,LITERAL(";")))
		{
			output_warning("%s(%d): libraries not yet supported", filename, linenum);
			/* TROUBLESHOOT
				An attempt to parse a <b>library</b> GLM directive was made.  Library directives
				are not supported yet.
			 */
			ACCEPT;
			DONE;
		}
		else
		{
			output_error_raw("%s(%d): library syntax error", filename, linenum);
			REJECT;
		}
	}
	REJECT;
}

static int comment_block(PARSER)
{
	int startline = linenum;
	if (_p[0]=='/' && _p[1]=='*')
	{
		int result=0;
		int matched=0;
		_p+=2;
		while (_p[0]!='*' && _p[1]=='/')
		{
			_p++; result++;
			if (_p[0]=='\0')
			{
				output_error_raw("%s(%d): unterminated C-style comment", filename, startline);
				return 0;
			}
			if (_p[0]=='\n')
				linenum++;
		}
		return matched?result:0;
	}
	return 0;
}

static int schedule(PARSER)
{
	int startline = linenum;
	char schedname[64];
	START;
	if WHITE ACCEPT;
	if (LITERAL("schedule") && WHITE && TERM(name(HERE,schedname,sizeof(schedname))) && WHITE,LITERAL("{"))
	{
		char buffer[65536], *p=buffer;
		int nest=0;
		for (nest=0; nest>=0; _m++)
		{
			char c = *HERE;
			if (c=='\0') break;
			switch (c) {
			case '{': nest++; *p++ = c; break;
			case '}': if (nest-->0) *p++ = c; break; 
			case '\n': *p++ = c; ++linenum; break;
			//case '\r': *p++ = c; ++linenum; break;
			default: *p++ = c; break;
			}
			*p = '\0';
		}
		if (schedule_create(schedname, buffer))
		{
			ACCEPT;
		}
		else
		{
			output_error_raw("%s(%d): schedule '%s' is not valid", filename, startline, schedname);
			REJECT;
		}
	}
	else
		REJECT;
	DONE;
}

static int linkage_term(PARSER,instance *inst)
{
	int startline = linenum;
	char fromobj[64];
	char fromvar[64];
	char toobj[64];
	char tovar[64];
	START;
	if WHITE ACCEPT;
	if ( TERM(name(HERE,fromobj,sizeof(fromobj))) && LITERAL(":") && TERM(name(HERE,fromvar,sizeof(fromvar))) 
		&& WHITE,LITERAL("->") && WHITE,TERM(name(HERE,toobj,sizeof(toobj))) && LITERAL(":") && TERM(name(HERE,tovar,sizeof(tovar)))
		&& LITERAL(";"))
	{
		if ( linkage_create_writer(inst,fromobj,fromvar,toobj,tovar) ) ACCEPT 
		else {
			output_error_raw("%s(%d): linkage to write '%s:%s' to '%s:%s' is not valid", filename, startline, fromobj, fromvar, toobj, tovar);
			REJECT;
		}
		DONE;
	}
	OR if ( TERM(name(HERE,toobj,sizeof(toobj))) && LITERAL(":") && TERM(name(HERE,tovar,sizeof(tovar))) 
		&& WHITE,LITERAL("<-") && WHITE,TERM(name(HERE,fromobj,sizeof(fromobj))) && LITERAL(":") && TERM(name(HERE,fromvar,sizeof(fromvar)))
		&& LITERAL(";"))
	{
		if ( linkage_create_reader(inst,fromobj,fromvar,toobj,tovar) ) ACCEPT 
		else {
			output_error_raw("%s(%d): linkage to read '%s:%s' from '%s:%s' is not valid", filename, startline, fromobj, fromvar, toobj, tovar);
			REJECT;
		}
		DONE;
	}
	OR if ( LITERAL("model") && WHITE && TERM(value(HERE,inst->model,sizeof(inst->model))) && WHITE,LITERAL(";"))
	{
		ACCEPT;
		DONE;
	}
	OR if ( LITERAL("cacheid") && WHITE && TERM(integer(HERE,&(inst->cacheid))) && WHITE,LITERAL(";"))
	{
		ACCEPT;
		DONE;
	}
	OR if ( LITERAL("mode") && WHITE && TERM(value(HERE,inst->cnxtypestr,sizeof(inst->cnxtypestr))) && WHITE, LITERAL(";"))
	{
		ACCEPT;
		DONE;
	}
	OR if ( LITERAL("execdir") && WHITE && TERM(value(HERE,inst->execdir,sizeof(inst->execdir))) && WHITE, LITERAL(";"))
	{
		ACCEPT;
		DONE;
	}
	OR if ( LITERAL("return_port") && WHITE && TERM(integer16(HERE,&(inst->return_port))) && WHITE, LITERAL(";"))
	{
		output_debug("linkage_term(): return_port = %d", inst->return_port);
		ACCEPT;
		DONE;
	}
	OR if ( LITERAL("}") )
	{
		REJECT;
	}
	OR
	{
		output_error_raw("%s(%d): unrecognized instance term at or after '%.10s...'", filename, startline, HERE);
		REJECT;
	}
	DONE;
	
}
static int instance_block(PARSER)
{
	int startline = linenum;
	char instance_host[256];
	START;
	if WHITE ACCEPT;
	if ( LITERAL("instance") && WHITE && TERM(hostname(HERE,instance_host,sizeof(instance_host))) && WHITE,LITERAL("{"))
	{
		instance *inst = instance_create(instance_host);
		if ( !inst ) 
		{ 
			output_error_raw("%s(%d): unable to define an instance on %s", filename, startline, instance_host);
			REJECT; 
			DONE; 
		}
		ACCEPT;
		while ( TERM(linkage_term(HERE,inst)) ) ACCEPT;
		if ( WHITE,LITERAL("}") ) { ACCEPT; DONE }
		else REJECT;
	}
	else
		REJECT;
	DONE;
}
////////////////////////////////////////////////////////////////////////////////////
// GUI parser

static int gnuplot(PARSER, GUIENTITY *entity)
{
	char *p = entity->gnuplot;
	int _n = 0;
	while ( _p[_n]!='}' )
	{
		if (_p[_n]=='\n') linenum++;
		*p++ = _p[_n++];
		if ( p>entity->gnuplot+sizeof(entity->gnuplot) )
		{
			output_error_raw("%s(%d): gnuplot script too long", filename, linenum);
			return _n;
		}
	}
	return _n;
}

static int gui_link_globalvar(PARSER, GLOBALVAR **var)
{
	char varname[64];
	START;
	if (LITERAL("link") && WHITE,LITERAL(":") && name(HERE,varname,sizeof(varname)))
	{
		*var = global_find(varname);
		ACCEPT;
	}
	else
		REJECT;
	DONE;
}

static int gui_entity_parameter(PARSER, GUIENTITY *entity)
{
	char buffer[1024];
	char varname[64];
	char modname[64];
	char objname[64];
	char propname[64];
	START;
	if WHITE ACCEPT;
	if LITERAL("global")
	{
		ACCEPT;
		if WHITE ACCEPT;
		if (TERM(name(HERE,modname,sizeof(modname))) && LITERAL("::") && TERM(name(HERE,varname,sizeof(varname))) && (WHITE,LITERAL(";")))
		{
			char fullname[256];
			ACCEPT;
			sprintf(fullname,"%s::%s",modname,varname);
			gui_set_variablename(entity,fullname);
			DONE;
		}
		OR if (TERM(name(HERE,varname,sizeof(varname))) && (WHITE,LITERAL(";")))
		{
			ACCEPT;
			gui_set_variablename(entity,varname);
			if (gui_get_variable(entity) || gui_get_environment(entity))
			{
				DONE;
			}
			else
			{
				output_error_raw("%s(%d): invalid gui global variable '%s'", filename, linenum,varname);
				REJECT;
			}
		}
		else
		{
			output_error_raw("%s(%d): invalid gui global variable specification", filename, linenum);
			REJECT;
		}
	}
	OR if LITERAL("link") 
	{
		ACCEPT;
		if WHITE ACCEPT;
		if (TERM(name(HERE,objname,sizeof(objname))) && LITERAL(":") && TERM(name(HERE,propname,sizeof(propname))) && WHITE,LITERAL(";"))
		{
			gui_set_objectname(entity,objname);
			gui_set_propertyname(entity,propname);
			ACCEPT; 
			DONE;
		}
		else
		{
			output_error_raw("%s(%d): invalid gui link object:property specification", filename, linenum);
			REJECT;
		}
	}
	OR if LITERAL("value") 
	{ 
		ACCEPT;
		if WHITE ACCEPT;
		if (TERM(value(HERE,buffer,sizeof(buffer))) && WHITE,LITERAL(";"))
		{
			gui_set_value(entity,buffer);
			ACCEPT; 
			DONE;
		}
		else
		{
			output_error_raw("%s(%d): invalid gui value specification", filename, linenum);
			REJECT;
		}
	}
	OR if LITERAL("source") 
	{ 
		ACCEPT;
		if WHITE ACCEPT;
		if (TERM(value(HERE,buffer,sizeof(buffer))) && WHITE,LITERAL(";"))
		{
			gui_set_source(entity,buffer);
			ACCEPT; 
			DONE;
		}
		else
		{
			output_error_raw("%s(%d): invalid gui value specification", filename, linenum);
			REJECT;
		}
	}
	OR if LITERAL("options") 
	{ 
		ACCEPT;
		if WHITE ACCEPT;
		if (TERM(value(HERE,buffer,sizeof(buffer))) && WHITE,LITERAL(";"))
		{
			gui_set_options(entity,buffer);
			ACCEPT; 
			DONE;
		}
		else
		{
			output_error_raw("%s(%d): invalid gui options specification", filename, linenum);
			REJECT;
		}
	}	OR if LITERAL("unit") 
	{ 
		ACCEPT;
		if WHITE ACCEPT;
		if (TERM(value(HERE,buffer,sizeof(buffer))) && WHITE,LITERAL(";"))
		{
			gui_set_unit(entity,buffer);
			if (gui_get_unit(entity))
			{
				ACCEPT; 
				DONE;
			}
			else
			{
				output_error_raw("%s(%d): invalid gui unit '%s'", filename, linenum, buffer);
				REJECT;
			}
		}
		else
		{
			output_error_raw("%s(%d): invalid gui unit specification", filename, linenum);
			REJECT;
		}
		ACCEPT;  
		DONE;
	}
	OR if LITERAL("size") 
	{ 
		ACCEPT;
		if WHITE ACCEPT;
		if (TERM(integer32(HERE,&entity->size)) && WHITE,LITERAL(";"))
		{
			ACCEPT; 
			DONE;
		}
		else
		{
			output_error_raw("%s(%d): invalid gui size specification", filename, linenum);
			REJECT;
		}
		ACCEPT;  
		DONE;
	}
	OR if LITERAL("height") 
	{ 
		ACCEPT;
		if WHITE ACCEPT;
		if (TERM(integer32(HERE,&entity->height)) && WHITE,LITERAL(";"))
		{
			ACCEPT; 
			DONE;
		}
		else
		{
			output_error_raw("%s(%d): invalid gui height specification", filename, linenum);
			REJECT;
		}
		ACCEPT;  
		DONE;
	}	
	OR if LITERAL("width") 
	{ 
		ACCEPT;
		if WHITE ACCEPT;
		if (TERM(integer32(HERE,&entity->width)) && WHITE,LITERAL(";"))
		{
			ACCEPT; 
			DONE;
		}
		else
		{
			output_error_raw("%s(%d): invalid gui width specification", filename, linenum);
			REJECT;
		}
		ACCEPT;  
		DONE;
	}
	OR if ( LITERAL("gnuplot") && WHITE,LITERAL("{") )
	{
		ACCEPT;
		if ( TERM(gnuplot(HERE,entity)) )
		{
			ACCEPT;
			if ( LITERAL("}") )
			{
				ACCEPT;
				DONE;
			}
			else
			{
				output_error_raw("%s(%d): missing closing } after gnuplot script", filename, linenum);
				REJECT;
			}
		}
		else
		{
			output_error_raw("%s(%d): invalid gnuplot script", filename, linenum);
			REJECT;
		}
	}
	OR if ( LITERAL("wait") && WHITE,TERM(value(HERE,entity->wait_for,sizeof(entity->wait_for))) && WHITE,LITERAL(";")  )
	{
		ACCEPT;
		DONE;
	}
	OR if ( LITERAL("hold") && WHITE,LITERAL(";") )
	{
		ACCEPT;
		entity->hold = 1;
		DONE;
	}
	REJECT;
}

static int gui_entity_action(PARSER, GUIENTITY *parent)
{
	START;
	if WHITE ACCEPT;
	if LITERAL("action")
	{
		GUIENTITY *entity = gui_create_entity();
		gui_set_type(entity,GUI_ACTION);
		gui_set_srcref(entity,filename,linenum);
		entity->parent = parent;
		ACCEPT;
		if WHITE ACCEPT;
		if (TERM(value(HERE,entity->action,sizeof(entity->action))) && WHITE,LITERAL(";"))
		{
			ACCEPT;
			DONE;
		}
		else {
			output_error_raw("%s(%d): invalid gui action specification", filename, linenum);
			REJECT;
		}
	}
	REJECT;
}

static int gui_entity_type(PARSER, GUIENTITYTYPE *type)
{
	START;
	if WHITE ACCEPT;
	// labeling entities
	if LITERAL("title") { ACCEPT; *type = GUI_TITLE; DONE; };
	if LITERAL("status") { ACCEPT; *type = GUI_STATUS; DONE; };
	if LITERAL("text") { ACCEPT; *type = GUI_TEXT; DONE; };
	// input entities
	if LITERAL("input") { ACCEPT; *type = GUI_INPUT; DONE; };
	if LITERAL("check") { ACCEPT; *type = GUI_CHECK; DONE; };
	if LITERAL("radio") { ACCEPT; *type = GUI_RADIO; DONE; };
	if LITERAL("select") { ACCEPT; *type = GUI_SELECT; DONE; };
	// output entities
	if LITERAL("browse") { ACCEPT; *type = GUI_BROWSE; DONE; };
	if LITERAL("table") { ACCEPT; *type = GUI_TABLE; DONE; };
	if LITERAL("graph") { ACCEPT; *type = GUI_GRAPH; DONE; };
	// grouping entities
	if LITERAL("row") { ACCEPT; *type = GUI_ROW; DONE; };
	if LITERAL("tab") { ACCEPT; *type = GUI_TAB; DONE; }; // beware not to put this before "table"
	if LITERAL("page") { ACCEPT; *type = GUI_PAGE; DONE; };
	if LITERAL("group") { ACCEPT; *type = GUI_GROUP; DONE; };
	if LITERAL("span") { ACCEPT; *type = GUI_SPAN; DONE; };
	REJECT;
}

static int gui_entity(PARSER, GUIENTITY *parent)
{
	//char buffer[1024];
	int type;
	START;
	if WHITE ACCEPT;
	if TERM(gui_entity_type(HERE,(GUIENTITYTYPE *)&type))
	{ 
		GUIENTITY *entity = gui_create_entity();
		gui_set_type(entity,type);
		gui_set_srcref(entity,filename,linenum);
		gui_set_parent(entity,parent);
		if WHITE ACCEPT;
		if LITERAL("{")
		{
			ACCEPT;
			while (true) {
				if WHITE ACCEPT;
				if (gui_is_grouping(entity) && TERM(gui_entity(HERE,entity))) 
				{
					ACCEPT; 
					continue; 
				}
				if (TERM(gui_entity_parameter(HERE,entity))) { ACCEPT; continue; }
				if (TERM(gui_entity_action(HERE,entity))) { ACCEPT; continue; }
				if LITERAL("}") { ACCEPT; break; }
				output_error_raw("%s(%d): unknown gui entity", filename, linenum);
				REJECT;
			} 
			DONE;
		}
		if TERM(gui_entity_parameter(HERE,entity)) { ACCEPT; DONE; }
		if (TERM(gui_entity_action(HERE,entity))) { ACCEPT; DONE; }
		if ( LITERAL(";" ) ) { ACCEPT; DONE; }
		REJECT;
	}
	REJECT;
}

static int gui(PARSER)
{
	START;
	if WHITE ACCEPT;
	if (LITERAL("gui") && WHITE,LITERAL("{"))
	{
		while TERM(gui_entity(HERE,NULL)) ACCEPT;
		if (WHITE,LITERAL("}")) 
		{
			if (gui_wait()==0)
			{
				output_error_raw("%s(%d): quit requested by user", filename, linenum);
				REJECT;
			}
			ACCEPT;
		}
	}
	else REJECT;
	DONE;
}

static int C_code_block(PARSER, char *buffer, int size)
{
	int n_curly = 0;
	int in_quotes = 0;
	int in_quote = 0;
	int in_comment = 0;
	int in_linecomment = 0;
	char *d = buffer;
	START;
	do 
	{
		char c = *_p;
		int skip=0;
		int ignore_curly = in_quotes || in_quote || in_comment || in_linecomment;
		switch (c) {
		case '{': if (!ignore_curly) n_curly++; break;
		case '}': if (!ignore_curly) n_curly--; break;
		case '/': if (_p[1]=='*') skip=1, in_comment=1; else if (_p[1]=='/') skip=1, in_linecomment=1; break;
		case '*': if (_p[1]=='/' && in_comment) skip=1, in_comment=0; break;
		case '\n': in_linecomment=0; linenum++; break;
		default: break;
		}
		*d++ = *_p;
		if (skip) _n++,*d++=*++_p;
	} while ( *++_p!='\0', _n++<size, n_curly>=0 );
	*--d='\0'; _n--; // don't include the last curly
//	output_debug("*** Begin external 'C' code ***\n%s\n *** End external 'C' code ***\n", buffer);
	DONE;
}

static int extern_block(PARSER)
{
	char code[65536];
	char libname[1024];
	char fnclist[4096];

	START;
	if WHITE ACCEPT;
	if ( LITERAL("extern") && WHITE,LITERAL("\"C\"") )
	{
		int startline=0;
		if WHITE ACCEPT;
		if ( TERM(name(HERE,libname,sizeof(libname))) && WHITE,LITERAL(":") && WHITE,TERM(namelist(HERE,fnclist,sizeof(fnclist))) )
		{
			ACCEPT;
		}
		else 
		{
			output_error_raw("%s(%d): missing library name and/or external function list", filename, linenum);
			REJECT;
		}
		if ( WHITE,LITERAL("{") && (WHITE,(startline=linenum),TERM(C_code_block(HERE,code,sizeof(code)))) && LITERAL("}") ) // C-code block
		{
			int rc = module_compile(libname,code,global_module_compiler_flags,
				"typedef struct { void *data, *info;} GLXDATA;\n"
				"#define GLXdouble(X) (*((double*)(X.data)))\n"
				/* TODO add external interface code before this line */,
				filename,startline-1);
			if ( rc==0 )
			{	
				if ( module_load_function_list(libname,fnclist) )
				{
					ACCEPT;
				}
				else
				{
					output_error_raw("%s(%d): unable to load inline functions '%s' from library '%s'", filename, linenum, fnclist, libname);
					REJECT;
				}
			}
			else
			{
				output_error_raw("%s(%d): module_compile error encountered (rc=%d)", filename, linenum, rc);
				REJECT;
			}
		}
		else if ( WHITE,LITERAL(";")	)
		{
			if ( module_load_function_list(libname,fnclist) )
			{
				ACCEPT;
			}
			else
			{
				output_error_raw("%s(%d): unable to load external functions '%s' from library '%s'", filename, linenum, fnclist, libname);
				REJECT;
			}
		}
		else
		{
			REJECT;
		}
	}
	DONE;
}

static int global_declaration(PARSER)
{
	START;
	if ( WHITE,LITERAL("global") )
	{
		char proptype[256];
		char varname[256];
		char pvalue[1024];
		if ( (WHITE,TERM(name(HERE,proptype,sizeof(proptype)))) 
			&& (WHITE,TERM(name(HERE,varname,sizeof(varname))))
			)
		{
			UNIT *pUnit = NULL;
			if ( (WHITE,LITERAL("[")) && (WHITE,TERM(unitspec(HERE,&pUnit))) && (WHITE,LITERAL("]")) )
			{
			}
			else
				pUnit = NULL;
			if ( (WHITE,TERM(value(HERE,pvalue,sizeof(pvalue)))) )
			{
				PROPERTYTYPE ptype = property_get_type(proptype);
				GLOBALVAR *var = global_create(varname,ptype,NULL,PT_SIZE,1,PT_ACCESS,PA_PUBLIC,NULL);
				if ( var==NULL )
				{
					output_error_raw("%s(%d): global '%s %s' cannot be defined", filename, linenum, proptype, varname);
					REJECT;
				}
				var->prop->unit = pUnit;
				if ( class_string_to_property(var->prop, var->prop->addr,pvalue)==0 )
				{
					output_error_raw("%s(%d): global '%s %s' cannot be set to '%s'", filename, linenum, proptype, varname, pvalue);
					REJECT;
				}
			}
		}
		ACCEPT;
		DONE;
	}
	else
		REJECT;
}

static int link_declaration(PARSER)
{
	START;
	if ( WHITE,LITERAL("link") )
	{
		char path[1024];
		if ( (WHITE,TERM(value(HERE,path,sizeof(path)))) && LITERAL(";") )
		{
			if ( !link_create(path) )
			{
				output_error_raw("%s(%d): unable to link '%s'", filename,linenum,path);
				REJECT;
			}
		}
		ACCEPT;
		DONE;
	}
	else
		REJECT;
}

////////////////////////////////////////////////////////////////////////////////////
static int script_directive(PARSER)
{
	START;
	if ( WHITE,LITERAL("script") )
	{
		char command[1024];
		if WHITE { ACCEPT; }
		if ( LITERAL("on_create") )
		{	if ( WHITE,TERM(value(HERE,command,sizeof(command))) && WHITE,LITERAL(";") )
			{
				if ( exec_add_createscript(command)==0 )
				{
					output_error_raw("%s(%d): unable to add on_create script '%s'", filename,linenum,command);
					REJECT;
				}
				else
				{
					ACCEPT; DONE;
				}
			}
			else
				REJECT;
		}
		if ( LITERAL("on_init") )
		{	
			if ( WHITE,TERM(value(HERE,command,sizeof(command))) && WHITE,LITERAL(";") )
			{
				if ( exec_add_initscript(command)==0 )
				{
					output_error_raw("%s(%d): unable to add on_init script '%s'", filename,linenum,command);
					REJECT;
				}
				else
				{
					ACCEPT; DONE;
				}
			}
			else
				REJECT;
		}
		if ( LITERAL("on_sync") )
		{	
			if ( WHITE,TERM(value(HERE,command,sizeof(command))) && WHITE,LITERAL(";") )
			{
				if ( exec_add_syncscript(command)==0 )
				{
					output_error_raw("%s(%d): unable to add on_sync script '%s'", filename,linenum,command);
					REJECT;
				}
				else
				{
					ACCEPT; DONE;
				}
			}
			else
				REJECT;
		}
		if ( LITERAL("on_term") )
		{
			if ( WHITE,TERM(value(HERE,command,sizeof(command))) && WHITE,LITERAL(";") )
			{
				if ( exec_add_termscript(command)==0 )
				{
					output_error_raw("%s(%d): unable to add on_term script '%s'", filename,linenum,command);
					REJECT;
				}
				else
				{
					ACCEPT; DONE;
				}
			}
			else
				REJECT;
		}
		if ( LITERAL("export") )
		{
			if ( WHITE,TERM(name(HERE,command,sizeof(command))) && WHITE,LITERAL(";") )
			{
				if ( exec_add_scriptexport(command)==0 )
				{
					output_error_raw("%s(%d): unable to export '%s'", filename,linenum,command);
					REJECT;
				}
				else
				{
					ACCEPT; DONE;
				}
			}
			else
				REJECT;
		}
		if ( TERM(value(HERE,command,sizeof(command))) && WHITE,LITERAL(";") )
		{
			int rc;
			output_verbose("running command [%s]", command);
			rc = system(command);
			if ( rc!=0 )
			{
				output_error_raw("%s(%d): script failed - return code %d", filename, linenum, rc);
				REJECT;
			}
			else
			{
				ACCEPT; DONE;
			}
		}
		else
		{
			REJECT;
		}
	}
	else
		REJECT;
}

////////////////////////////////////////////////////////////////////////////////////

static int gridlabd_file(PARSER)
{
	START;
	if WHITE {ACCEPT; DONE;}
	OR if LITERAL(";") {ACCEPT; DONE;}
	OR if TERM(line_spec(HERE)) { ACCEPT; DONE; }
	OR if TERM(object_block(HERE,NULL,NULL)) {ACCEPT; DONE;}
	OR if TERM(class_block(HERE)) {ACCEPT; DONE;}
	OR if TERM(module_block(HERE)) {ACCEPT; DONE;}
	OR if TERM(clock_block(HERE)) {ACCEPT; DONE;}
	OR if TERM(import(HERE)) {ACCEPT; DONE; }
	OR if TERM(library(HERE)) {ACCEPT; DONE; }
	OR if TERM(schedule(HERE)) {ACCEPT; DONE; }
	OR if TERM(instance_block(HERE)) {ACCEPT; DONE; }
	OR if TERM(gui(HERE)) {ACCEPT; DONE;}
	OR if TERM(extern_block(HERE)) {ACCEPT; DONE; }
	OR if TERM(global_declaration(HERE)) {ACCEPT; DONE; }
	OR if TERM(link_declaration(HERE)) { ACCEPT; DONE; }
	OR if TERM(script_directive(HERE)) { ACCEPT; DONE; }
	OR if (*(HERE)=='\0') {ACCEPT; DONE;}
	else REJECT;
	DONE;
}

int replace_variables(char *to,char *from,int len,int warn)
{
	char *p, *e=from;
	int n = 0;
	while ((p=strstr(e,"${"))!=NULL)
	{
		char varname[1024];
		if (sscanf(p+2,"%1024[^}]",varname)==1)
		{
			char *env = getenv(varname);
			char *var;
			int m = (int)(p-e);
			strncpy(to+n,e,m);
			n += m;
			var =  global_getvar(varname,to+n,len-n);
			if (var!=NULL)
				n+=(int)strlen(var);
			else if (env!=NULL)
			{
				strncpy(to+n,env,len-n);
				n+=(int)strlen(env);
			}
			else if ( warn )
			{
				/* this must be benign because otherwise macros that are inactive fail when they shouldn't */
				output_warning("%s(%d): variable '%s' not found", filename, linenum, varname);
				/* TROUBLESHOOT
					A macro refers to a variable that is not defined.  Correct the variable reference, or
					define the variable before using it and try again.
				 */
			}
			e = strchr(p,'}');
			if (e==NULL)
				goto Unterminated;
			e++;
		}
		else
		{
Unterminated:
			output_error_raw("%s(%d): unterminated variable name %.10p...", filename, linenum, p);
			return 1;
		}
	}

	if ((int)strlen(e)<len-n)
	{
		strcpy(to+n,e);
		return (int)strlen(to);
	}
	else
	{
		output_error_raw("%s(%d): insufficient buffer space to continue", filename, linenum);
		return -1;
	}
}

static int suppress = 0;
static int nesting = 0;
static int macro_line[64];
static int process_macro(char *line, int size, char *filename, int linenum);
static int buffer_read(FILE *fp, char *buffer, char *filename, int size)
{
	char line[65536];
	int n=0;
	int linenum=0;
	int startnest = nesting;
	while (fgets(line,sizeof(line),fp)!=NULL)
	{
		int len;
		char subst[65536];

		/* comments must have preceding whitespace in macros */
		char *c = line[0]!='#'?strstr(line,COMMENT):strstr(line, " " COMMENT);
		linenum++;
		if (c!=NULL) /* truncate at comment */
			strcpy(c,"\n");
		len = (int)strlen(line);
		if (len>=size-1)
			return 0;

	
#ifndef OLDSTYLE
		/* check for oldstyle file under newstyle parse */
		if (linenum==1 && strncmp(line,"# ",2)==0)
		{
			output_error("%s looks like a version 1.x GLM files, please convert this file to new style before loading", filename);
			return 0;
		}
#endif
		/* expand variables */
		if ((len=replace_variables(subst,line,sizeof(subst),suppress==0))>=0)
			strcpy(line,subst);
		else
		{
			output_error_raw("%s(%d): unable to continue", filename,linenum);
			return -1;
		}

		/* expand macros */
		if (strncmp(line,MACRO,strlen(MACRO))==0)
		{
			/* macro disables reading */
			if (process_macro(line,sizeof(line),filename,linenum)==FALSE)
				return 0;
			len = (int)strlen(line);
			strcat(buffer,line);
			buffer += len;
			size -= len;
			n += len;
		}

		/* if reading is enabled */
		else if (suppress==0)
		{
			strcpy(buffer,subst);
			buffer+=len;
			size -= len;
			n+=len;
		}
	}
	if (nesting != startnest)
	{
		//output_message("%s(%d): missing %sendif for #if at %s(%d)", filename,linenum,MACRO,filename,macro_line[nesting-1]);
		output_error_raw("%s(%d): Unbalanced %sif/%sendif at %s(%d) ~ started with nestlevel %i, ending %i", filename,linenum,MACRO,MACRO,filename,macro_line[nesting-1], startnest, nesting);
		return -1;
	}
	return n;
}

static int buffer_read_alt(FILE *fp, char *buffer, char *filename, int size)
{
	char line[10240];
	char *buf = buffer;
	int n = 0, i = 0;
	int _linenum=0;
	int startnest = nesting;
	int bnest = 0, quote = 0;
	int hassc = 0; // has semicolon
	int quoteline = 0;
	while (fgets(line,sizeof(line),fp)!=NULL)
	{
		int len;
		char subst[65536];

		/* comments must have preceding whitespace in macros */
		char *c = line[0]!='#'?strstr(line,COMMENT):strstr(line, " " COMMENT);
		_linenum++;
		if (c!=NULL) /* truncate at comment */
			strcpy(c,"\n");
		len = (int)strlen(line);
		if (len>=size-1){
			output_error("load.c: buffer exhaustion reading %i lines past line %i", _linenum, linenum);
			if(quote != 0){
				output_error("look for an unterminated doublequote string on line %i", quoteline);
			}
			return 0;
		}
	
#ifndef OLDSTYLE
		/* check for oldstyle file under newstyle parse */
		if (_linenum==1 && strncmp(line,"# ",2)==0)
		{
			output_error("%s looks like a version 1.x GLM files, please convert this file to new style before loading", filename);
			return 0;
		}
#endif
		/* expand variables */
		if ((len=replace_variables(subst,line,sizeof(subst),suppress==0))>=0)
			strcpy(line,subst);
		else
		{
			output_error_raw("%s(%d): unable to continue", filename,_linenum);
			return -1;
		}

		/* expand macros */
		if (strncmp(line,MACRO,strlen(MACRO))==0)
		{
			/* macro disables reading */
			if (process_macro(line,sizeof(line),filename,linenum + _linenum - 1)==FALSE){
				return 0;
			} else {
				++hassc;
			}
			//strcat(buffer,line);
			strcpy(buffer,line);
			len = (int)strlen(buffer); // include anything else in the buffer, then advance
			buffer += len;
			size -= len;
			n += len;
		}

		/* if reading is enabled */
		else if (suppress==0)
		{
			strcpy(buffer,subst);
			buffer+=len;
			size -= len;
			n+=len;
			for(i = 0; i < len; ++i){
				if(quote == 0){
					if(subst[i] == '\"'){
						quoteline = linenum + _linenum - 1;
						quote = 1;
					} else if(subst[i] == '{'){
						++bnest;
						++hassc;
						// @TODO push context
					} else if(subst[i] == '}'){
						--bnest;
						// @TODO pop context
					} else if(subst[i] == ';'){
						++hassc;
					}
				} else {
					if(subst[i] == '\"'){
						quote = 0;
					}
				}
			}
		} else {
			strcpy(buffer,"\n");
			buffer+=strlen("\n");
			size -= 1;
			n += 1;
		}
		if(bnest == 0 && hassc > 0 && nesting == startnest){ // make sure we read ALL of an #if block, if possible
			/* end of block */
			return n;
		}

	}
	if(quote != 0){
		output_warning("unterminated doublequote string");
	}
	if(bnest != 0){
		output_warning("incomplete loader block");
	}
	if (nesting != startnest)
	{
		//output_message("%s(%d): missing %sendif for #if at %s(%d)", filename,_linenum,MACRO,filename,macro_line[nesting-1]);
		output_error_raw("%s(%d): Unbalanced %sif/%sendif at %s(%d) ~ started with nestlevel %i, ending %i", filename,_linenum,MACRO,MACRO,filename,macro_line[nesting-1], startnest, nesting);
		return -1;
	}
	return n;
}


static int include_file(char *incname, char *buffer, int size, int _linenum)
{
	int move = 0;
	char *p = buffer;
	int count = 0;
	char *ext = 0;
	char *name = 0;
	STAT stat;
	char ff[1024];
	FILE *fp = 0;
	char buffer2[20480];
	unsigned int old_linenum = _linenum;
	/* check include list */
	INCLUDELIST *list;
	INCLUDELIST *this = (INCLUDELIST *)malloc(sizeof(INCLUDELIST));//={incname,include_list}; /* REALLY BAD IDEA ~~ "this" is a reserved C++ keyword */

	strcpy(this->file, incname);
	this->next = include_list;

	buffer2[0]=0;
	
	for (list = include_list; list != NULL; list = list->next)
	{
		if (strcmp(incname, list->file) == 0)
		{
			output_error_raw("%s(%d): include file has already been included", incname, _linenum);
			return 0;
		}
	}

	/* if source file, add to header list and keep moving */
	ext = strrchr(incname, '.');
	name = strrchr(incname, '/');
	if (ext>name) {
		if(strcmp(ext, ".hpp") == 0 || strcmp(ext, ".h")==0 || strcmp(ext, ".c") == 0 || strcmp(ext, ".cpp") == 0){
			// append to list
			for (list = header_list; list != NULL; list = list->next){
				if(strcmp(incname, list->file) == 0){
					// normal behavior
					return 0;
				}
			}
			this->next = header_list;
			header_list = this;
		}
	} else { /* no extension */
		for (list = header_list; list != NULL; list = list->next){
			if(strcmp(incname, list->file) == 0){
				// normal behavior
				return 0;
			}
		}
		this->next = header_list;
		header_list = this;
	}

	/* open file */
	fp = find_file(incname,NULL,R_OK,ff,sizeof(ff)) ? fopen(ff, "rt") : NULL;
	
	if(fp == NULL){
		output_error_raw("%s(%d): include file open failed: %s", incname, _linenum, errno?strerror(errno):"(no details)");
		return -1;
	}
	else
		output_verbose("include_file(char *incname='%s', char *buffer=0x%p, int size=%d): search of GLPATH='%s' result is '%s'", 
			incname, buffer, size, getenv("GLPATH") ? getenv("GLPATH") : "NULL", ff ? ff : "NULL");

	old_linenum = linenum;
	linenum = 1;

	if(FSTAT(fileno(fp), &stat) == 0){
		if(stat.st_mtime > modtime){
			modtime = stat.st_mtime;
		}

		//if(size < stat.st_size){
			/** @todo buffer must grow (ticket #31) */
			/* buffer = realloc(buffer,size+stat.st_size); */
		//	output_message("%s(%d): unable to grow size of read buffer to include file", incname, linenum);
		//	return 0;
		//}
	} else {
		output_error_raw("%s(%d): unable to get size of included file", incname, _linenum);
		return -1;
	}

	output_verbose("%s(%d): included file is %d bytes long", incname, old_linenum, stat.st_size);

	/* reset line counter for parser */
	include_list = this;
	//count = buffer_read(fp,buffer,incname,size); // fread(buffer,1,stat.st_size,fp);

	move = buffer_read_alt(fp, buffer2, incname, 20479);
	while(move > 0){
		count += move;
		p = buffer2; // grab a block
		while(*p != 0){
			// and process it
			move = gridlabd_file(p);
			if(move == 0)
				break;
			p += move;
		}
		if(*p != 0){
			// failed if we didn't parse the whole thing
			count = -1;
			break;
		}
		move = buffer_read_alt(fp, buffer2, incname, 20479);
	}

	//include_list = this.next;

	linenum = old_linenum;

	return count;
}

/** @return 1 if the variable is autodefined */
int is_autodef(char *value)
{
#ifdef WIN32
	if ( strcmp(value,"WINDOWS")==0 ) return 1;
#elif defined APPLE
	if ( strcmp(value,"APPLE")==0 ) return 1;
#elif defined LINUX
	if ( strcmp(value,"LINUX")==0 ) return 1;
#endif

#ifdef _DEBUG
	if ( strcmp(value,"DEBUG")==0 ) return 1;
#endif

#ifdef HAVE_MATLAB
	if ( strcmp(value,"MATLAB")==0 ) return 1;
#endif

#ifdef HAVE_XERCES
	if ( strcmp(value,"XERCES")==0 ) return 1;
#endif

#ifdef HAVE_CPPUNIT
	if ( strcmp(value,"CPPUNIT")==0 ) return 1;
#endif

	return 0;
}

/** @return TRUE/SUCCESS for a successful macro read, FALSE/FAILED on parse error (which halts the loader) */
static int process_macro(char *line, int size, char *_filename, int linenum)
{
#ifndef WIN32
	char *var, *val, *save;	// used by *nix
	int i, count;			// used by *nix
#endif
	char buffer[64];
	if (strncmp(line,MACRO "endif",6)==0)
	{
		if (nesting>0)
		{
			// @TODO pop 'if' context
			nesting--;
			suppress &= ~(1<<nesting);
		}
		else{
			output_error_raw("%s(%d): %sendif is mismatched", filename, linenum,MACRO);
		}
		strcpy(line,"\n");

		return TRUE;
	}
	else if (strncmp(line,MACRO "else",5)==0)
	{
		char *term;

		// @TODO pop 'if' context (old context)
		// @TODO push 'if' context (else context)

		if ( (suppress&(1<<(nesting-1))) == (1<<(nesting-1)) ){
			suppress &= ~(1<<(nesting-1));
		} else {
			suppress |= (1<<(nesting-1));
		}
		term = line+5;
		strip_right_white(term);
		if(strlen(term)!=0)
		{
			output_error_raw("%s(%d): %selse macro should not contain any terms",filename,linenum,MACRO);
			return FALSE;
		}
		strcpy(line,"\n");
		return TRUE;
	}
	else if (strncmp(line,MACRO "ifdef",6)==0)
	{
		char *term = strchr(line+6,' ');
		char value[1024];
		if (term==NULL)
		{
			output_error_raw("%s(%d): %sifdef macro missing term",filename,linenum,MACRO);
			return FALSE;
		}
		//if (sscanf(term+1,"%[^\n\r]",value)==1 && global_getvar(value, buffer, 63)==NULL && getenv(value)==NULL)
		strcpy(value, strip_right_white(term+1));
		if ( !is_autodef(value) && global_getvar(value, buffer, 63)==NULL && getenv(value)==NULL){
			suppress |= (1<<nesting);
		}
		macro_line[nesting] = linenum;
		nesting++;
		// @TODO push 'if' context

		strcpy(line,"\n");
		return TRUE;
	}
	else if (strncmp(line,MACRO "ifexist",8)==0)
	{
		char *term = strchr(line+8,' ');
		char value[1024];
		char path[1024];
		if (term==NULL)
		{
			output_error_raw("%s(%d): %sifexist macro missing term",filename,linenum,MACRO);
			return FALSE;
		}
		while(isspace((unsigned char)(*term)))
			++term;
		//if (sscanf(term,"\"%[^\"\n]",value)==1 && find_file(value, NULL, 0)==NULL)
		strcpy(value, strip_right_white(term));
		if(value[0] == '"'){
			char stripbuf[1024];
			sscanf(value, "\"%[^\"\n]", stripbuf);
			strcpy(value, stripbuf);
		}
		if (find_file(value, NULL, F_OK, path,sizeof(path))==NULL)
			suppress |= (1<<nesting);
		macro_line[nesting] = linenum;
		nesting++;
		// @TODO push 'file' context
		strcpy(line,"\n");
		return TRUE;
	}
	else if (strncmp(line,MACRO "ifndef",7)==0)
	{
		char *term = strchr(line+7,' ');
		char value[1024];
		if (term==NULL)
		{
			output_error_raw("%s(%d): %sifndef macro missing term",filename,linenum,MACRO);
			return FALSE;
		}
		//if (sscanf(term+1,"%[^\n\r]",value)==1 && global_getvar(value, buffer, 63)!=NULL || getenv(value)!=NULL))
		strcpy(value, strip_right_white(term+1));
		if(global_getvar(value, buffer, 63)!=NULL || getenv(value)!=NULL){
			suppress |= (1<<nesting);
		}
		macro_line[nesting] = linenum;
		nesting++;
		// @TODO push 'if' context
		strcpy(line,"\n");
		return TRUE;
	}
	else if (strncmp(line,MACRO "if",3)==0)
	{
		char var[32], op[4], *value;
		char val[1024];
		if (sscanf(line+4,"%[a-zA-Z_0-9]%[!<>=]%[^\n]",var,op,val)!=3)
		{
			output_error_raw("%s(%d): %sif macro statement syntax error", filename,linenum,MACRO);
			strcpy(line,"\n");
			return FALSE;
		}
		value = global_getvar(var, buffer, 63);
		if (value==NULL)
		{
			output_error_raw("%s(%d): %s is not defined", filename,linenum,var);
			strcpy(line,"\n");
			return FALSE;
		}
		if (strcmp(op,"<")==0) { if (!(strcmp(value,val)<0)) suppress|=(1<<nesting); }
		else if (strcmp(op,">")==0) { if (!(strcmp(value,val)>0)) suppress|=(1<<nesting); }
		else if (strcmp(op,">=")==0) { if (!(strcmp(value,val)>=0)) suppress|=(1<<nesting); }
		else if (strcmp(op,"<=")==0) { if (!(strcmp(value,val)<=0)) suppress|=(1<<nesting); }
		else if (strcmp(op,"==")==0) { if (!(strcmp(value,val)==0)) suppress|=(1<<nesting); }
		else if (strcmp(op,"!=")==0) { if (!(strcmp(value,val)!=0)) suppress|=(1<<nesting); }
		else
		{
			output_error_raw("%s(%d): operator %s is not recognized", filename,linenum,op);
			strcpy(line,"\n");
			return FALSE;
		}
		macro_line[nesting] = linenum;
		nesting++;
		// @TODO push 'if' context
		strcpy(line,"\n");
		return TRUE;
	}

	/* handles suppressed macros */
	if (suppress!=0)
	{
		strcpy(line,"\n");
		return TRUE;
	}

	/* these macros can be suppressed */
	if (strncmp(line,MACRO "include",8)==0)
	{
		char *term = strchr(line+8,' ');
		char value[1024];
		char oldfile[1024];
		if (term==NULL)
		{
			output_error_raw("%s(%d): %sinclude macro missing term",filename,linenum,MACRO);
			strcpy(line,"\n");
			return FALSE;
		}
		while(isspace((unsigned char)(*term)))
			++term;
		if (sscanf(term,"\"%[^\"]\"",value)==1)
		{
			char *start=line;
			int len = sprintf(line,"@%s;%d\n",value,0);
			line+=len; size-=len;
			strcpy(oldfile, filename);	// push old filename
			strcpy(filename, value);	// use include file name for errors while within context
			len=(int)include_file(value,line,size,linenum);
			strcpy(filename, oldfile);	// pop include filename, use calling filename
			if (len<0)
			{
				output_error_raw("%s(%d): #include failed",filename,linenum);
				include_fail = 1;
				strcpy(line,"\n");
				return FALSE;
			}
			else
			{
//				line+=len; size-=len; // not relevant to the block loader, was already consumed
				len = sprintf(line,"@%s;%d\n",filename,linenum);
				line+=len; size-=len;
				return size>0;
			}
		} else if(sscanf(term, "<%[^>]>", value) == 1){
			/* C include file */
			output_verbose("added C include for \"%s\"", value);
			append_code("#include <%s>\n",value);
			strcpy(line,"\n");
			return TRUE;
		}
		else if ( sscanf(term, "[%[^]]]", value)==1 )
		{
			/* HTTP include */
			int len=0;
			char *p;
			FILE *fp;
			HTTPRESULT *http = http_read(value,0x40000);
			char tmpname[1024];
			if ( http==NULL )
			{
				output_error("%s(%d): unable to include [%s]", filename, linenum, value);
				return FALSE;
			}
			
			/* local cache file name */
			len = sprintf(line,"@%s;%d\n",value,0);
			size -= len; line += len;
			strcpy(tmpname,value);
			for ( p=tmpname ; *p!='\0' ; p++ )
			{
				if ( isalnum(*p) || *p=='.' || *p=='-' || *p==',' || *p=='_' ) continue;
				*p = '_';
			}

			/* copy to local file - TODO check time stamps */
			if ( access(tmpname,R_OK)!=0 )
			{
				fp = fopen(tmpname,"wt");
				if ( fp==NULL )
				{
					output_error("%s(%d): unable to write temp file '%s'", filename, linenum, tmpname);
					return FALSE;
				}
				fwrite(http->body.data,1,http->body.size,fp);
				fclose(fp);
			}

			/* load temp file */
			strcpy(oldfile,filename);
			strcpy(filename,tmpname);
			len = (int)include_file(tmpname,line,size,linenum);
			strcpy(filename,oldfile);
			if ( len<0 )
			{
				output_error("%s(%d): unable to include load [%s] from temp file '%s'", filename, linenum, value,tmpname);
				return FALSE;
			}
			else
			{
				sprintf(line+len,"@%s;%d\n",filename,linenum);
				return TRUE;
			}
		}
		else
		{
			output_error_raw("%s(%d): #include failed",filename,linenum);
			strcpy(line,"\n");
			return FALSE;
		}
	}
	else if (strncmp(line,MACRO "setenv",7)==0)
	{
		char *term = strchr(line+7,' ');
		char value[65536];
		if (term==NULL)
		{
			output_error_raw("%s(%d): %ssetenv macro missing term",filename,linenum,MACRO);
			strcpy(line,"\n");
			return FALSE;
		}
		//if (sscanf(term+1,"%[^\n\r]",value)==1)
		strcpy(value, strip_right_white(term+1));
		if(1){
#ifdef WIN32
			putenv(value);
#else
			var = strtok_r(value, "=", &save);
                        val = strtok_r(NULL, "=", &save);
                        setenv(var, val, 1);
#endif
			strcpy(line,"\n");
			return SUCCESS;
		}
		else
		{
			output_error_raw("%s(%d): %ssetenv term missing or invalid",filename,linenum,MACRO);
			strcpy(line,"\n");
			return FALSE;
		}
	}
	else if (strncmp(line,MACRO "set",4)==0)
	{
		char *term = strchr(line+4,' ');
		char value[1024];
		if (term==NULL)
		{
			output_error_raw("%s(%d): %sset macro missing term",filename,linenum,MACRO);
			strcpy(line,"\n");
			return FALSE;
		}
		//if (sscanf(term+1,"%[^\n\r]",value)==1)
		strcpy(value, strip_right_white(term+1));
		if(1){
			STATUS result;
			if (strchr(value,'=')==NULL)
			{
				output_error_raw("%s(%d): %sset missing assignment",filename,linenum,MACRO);
				return FAILED;
			}
			else
			{
				int oldstrict = global_strictnames;
				global_strictnames = TRUE;
				result = global_setvar(value);
				global_strictnames = strncmp(value,"strictnames=",12)==0 ? global_strictnames : oldstrict;
				if (result==FAILED)
					output_error_raw("%s(%d): %sset term not found",filename,linenum,MACRO);
				strcpy(line,"\n");
				return result==SUCCESS;
			}
		}
		else
		{
			output_error_raw("%s(%d): %sset term missing or invalid",filename,linenum,MACRO);
			strcpy(line,"\n");
			return FALSE;
		}
	}
	else if (strncmp(line,MACRO "binpath",8)==0)
	{
		char *term = strchr(line+8,' ');
		char value[1024];
		if (term==NULL)
		{
			output_error_raw("%s(%d): %sbinpath macro missing term",filename,linenum,MACRO);
			strcpy(line,"\n");
			return FALSE;
		}
		//if (sscanf(term+1,"%[^\n\r]",value)==1)
		strcpy(value, strip_right_white(term+1));
		if(1){
			char path[1024];
			sprintf(path,"PATH=%s",value);
#ifdef WIN32
                        putenv(path);
#else
                        setenv("PATH", value, 1);
#endif

			strcpy(line,"\n");
			return SUCCESS;
		}
		else
		{
			output_error_raw("%s(%d): %sbinpath term missing or invalid",filename,linenum,MACRO);
			strcpy(line,"\n");
			return FALSE;
		}
	}
	else if (strncmp(line,MACRO "libpath",8)==0)
	{
		char *term = strchr(line+8,' ');
		char value[1024];
		if (term==NULL)
		{
			output_error_raw("%s(%d): %slibpath macro missing term",filename,linenum,MACRO);
			strcpy(line,"\n");
			return FALSE;
		}
		//if (sscanf(term+1,"%[^\n\r]",value)==1)
		strcpy(value, strip_right_white(term+1));
		if(1){
			char path[1024];
			sprintf(path,"GLPATH=%s",value);
#ifdef WIN32
                        putenv(path);
#else
                        setenv("GLPATH", value, 1);
#endif
			strcpy(line,"\n");
			return SUCCESS;
		}
		else
		{
			output_error_raw("%s(%d): %slibpath term missing or invalid",filename,linenum, MACRO);
			strcpy(line,"\n");
			return FALSE;
		}
	}
	else if (strncmp(line,MACRO "incpath",8)==0)
	{
		char *term = strchr(line+8,' ');
		char value[1024];
		if (term==NULL)
		{
			output_error_raw("%s(%d): %sincpath macro missing term",filename,linenum,MACRO);
			strcpy(line,"\n");
			return FALSE;
		}
		//if (sscanf(term+1,"%[^\n\r]",value)==1)
		strcpy(value, strip_right_white(term+1));
		if(1){
			char path[1024];
			sprintf(path,"INCLUDE=%s",value);
#ifdef WIN32
                        putenv(path);
#else
                        setenv("INCLUDE", value, 1);
#endif

			strcpy(line,"\n");
			return SUCCESS;
		}
		else
		{
			output_error_raw("%s(%d): %sincpath term missing or invalid",filename,linenum, MACRO);
			strcpy(line,"\n");
			return FALSE;
		}
	}
	else if (strncmp(line,MACRO "define",7)==0)
	{
		char *term = strchr(line+7,' ');
		char value[1024];
		if (term==NULL)
		{
			output_error_raw("%s(%d): %sdefine macro missing term",filename,linenum, MACRO);
			strcpy(line,"\n");
			return FALSE;
		}
		//if (sscanf(term+1,"%[^\n\r]",value)==1)
		strcpy(value, strip_right_white(term+1));
		if(1){
			STATUS result;
			int oldstrict = global_strictnames;
			if (strchr(value,'=')==NULL)
				strcat(value,"="); // void entry
			global_strictnames = FALSE;
			result = global_setvar(value,"\"\""); // extra "" is used in case value is term is empty string
			global_strictnames = oldstrict;
			if (result==FAILED)
				output_error_raw("%s(%d): %sdefine term not found",filename,linenum,MACRO);
			strcpy(line,"\n");
			return result==SUCCESS;
		}
		else
		{
			output_error_raw("%s(%d): %sdefine missing expression",filename,linenum,MACRO);
			strcpy(line,"\n");
			return FALSE;
		}
	}
	else if (strncmp(line,MACRO "print",6)==0)
	{
		char *term = strchr(line+6,' ');
		char value[1024];
		if (term==NULL)
		{
			output_error_raw("%s(%d): %sprint missing message text",filename,linenum,MACRO);
			strcpy(line,"\n");
			return FALSE;
		}
		//if (sscanf(term+1,"%[^\n\r]",value)==1)
		strcpy(value, strip_right_white(term+1));
		if(1){
			output_error_raw("%s(%d): %s", filename, linenum, value);
			strcpy(line,"\n");
			return TRUE;
		}
		else
		{
			output_error_raw("%s(%d): %sprint term not found",filename,linenum,MACRO);
			strcpy(line,"\n");
			return FALSE;
		}
	}
	else if (strncmp(line,MACRO "error",6)==0)
	{
		char *term = strchr(line+6,' ');
		char value[1024];
		if (term==NULL)
		{
			output_error_raw("%s(%d): %serror missing expression",filename,linenum,MACRO);
			strcpy(line,"\n");
			return FALSE;
		}
		//if (sscanf(term+1,"%[^\n\r]",value)==1)
		strcpy(value, strip_right_white(term+1));
		if(1){
			//output_message("%s(%d): ERROR - %s", filename, linenum, value);
			output_error_raw("%s(%d):\t%s", filename, linenum, value);
			strcpy(line,"\n");
			return FALSE;
		}
		else
		{
			output_error_raw("%s(%d): %serror missing message text",filename,linenum,MACRO);
			strcpy(line,"\n");
			return FALSE;
		}
	}
	else if (strncmp(line,MACRO "warning",8)==0)
	{
		char *term = strchr(line+8,' ');
		char value[1024];
		if (term==NULL)
		{
			output_error_raw("%s(%d): %swarning missing message text",filename,linenum,MACRO);
			strcpy(line,"\n");
			return FALSE;
		}
		//if (sscanf(term+1,"%[^\n\r]",value)==1)
		strcpy(value, strip_right_white(term+1));
		if(1){
			output_error_raw("%s(%d): WARNING - %s", filename, linenum, value);
			strcpy(line,"\n");
			return TRUE;
		}
		else
		{
			output_error_raw("%s(%d): %swarning missing expression",filename,linenum,MACRO);
			strcpy(line,"\n");
			return FALSE;
		}
	}
	else if (strncmp(line,MACRO "system",7)==0)
	{
		char *term = strchr(line+7,' ');
		char value[1024];
		if (term==NULL)
		{
			output_error_raw("%s(%d): %ssystem missing system call",filename,linenum,MACRO);
			strcpy(line,"\n");
			return FALSE;
		}
		strcpy(value, strip_right_white(term+1));
		output_debug("%s(%d): executing system(char *cmd='%s')", filename, linenum, value);
		global_return_code = system(value);
		if( global_return_code==127 || global_return_code==-1 )
		{
			output_error_raw("%s(%d): ERROR unable to execute '%s' (status=%d)", filename, linenum, value, global_return_code);
			strcpy(line,"\n");
			return FALSE;
		}
		else
		{
			strcpy(line,"\n");
			return TRUE;
		}
	}
	else if ( strncmp(line,MACRO "option",7)==0 )
	{
		char *term = strchr(line+7,' ');
		char value[1024];
		if (term==NULL)
		{
			output_error_raw("%s(%d): %soption missing command option name",filename,linenum,MACRO);
			strcpy(line,"\n");
			return FALSE;
		}
		strcpy(value, strip_right_white(term+1));
		strcpy(line,"\n");
		return cmdarg_runoption(value)>=0;
	}
	else
	{
		strcpy(line,"\n");
		return FALSE;
	}

	output_error_raw("%s(%d): macro fell out of logic tree", filename, linenum);
	return FALSE;
}

STATUS loadall_glm(char *file) /**< a pointer to the first character in the file name string */
{
	OBJECT *obj, *first = object_get_first();
	char *buffer = NULL, *p = NULL;
	int fsize = 0;
	STATUS status=FAILED;
	STAT stat;
	char *ext = strrchr(file,'.');
	FILE *fp;
	int move=0;
	errno = 0;

	fp = fopen(file,"rt");
	if (fp==NULL)
		goto Failed;
	if (FSTAT(fileno(fp),&stat)==0)
	{
		modtime = stat.st_mtime;
		fsize = stat.st_size;
		buffer = malloc(BUFFERSIZE); /* lots of space */
	}
	output_verbose("file '%s' is %d bytes long", file,fsize);
	if (buffer==NULL)
	{
		output_error("unable to allocate buffer for file '%s': %s", file, errno?strerror(errno):"(no details)");
		errno = ENOMEM;
		goto Done;
	}
	else
		p=buffer;

	buffer[0] = '\0';
	if (buffer_read(fp,buffer,file,BUFFERSIZE)==0)
	{
		fclose(fp);
		goto Failed;
	}
	fclose(fp);

	/* reset line counter for parser */
	linenum = 1;
	while (*p!='\0')
	{
		move = gridlabd_file(p);
		if (move==0)
			break;
		p+=move;
	}
	status = (*p=='\0') ? SUCCESS : FAILED;
	if (status==FAILED)
	{
		char *eol = strchr(p,'\n');
		if (eol!=NULL)
			*eol='\0';
		output_error_raw("%s(%d): load failed at or near '%.12s...'", file, linenum,*p=='\0'?"end of line":p);
		if (p==0)
			output_error("%s doesn't appear to be a GLM file", file);
		goto Failed;
	}
	else if ((status=load_resolve_all())==FAILED)
		goto Failed;

	/* establish ranks */
	for (obj=first?first:object_get_first(); obj!=NULL; obj=obj->next)
		object_set_parent(obj,obj->parent);
	output_verbose("%d object%s loaded", object_get_count(), object_get_count()>1?"s":"");
	goto Done;
Failed:
	if (errno!=0){
		output_error("unable to load '%s': %s", file, errno?strerror(errno):"(no details)");
		/*	TROUBLESHOOT
			In most cases, strerror(errno) will claim "No such file or directory".  This claim should be ignored in
			favor of prior error messages.
		*/
	}
Done:
	free(buffer);
	buffer = NULL;
	free_index();
	linenum=1; // parser starts at 1
	return status;
}

/**/
STATUS loadall_glm_roll(char *file) /**< a pointer to the first character in the file name string */
{
	OBJECT *obj, *first = object_get_first();
	//char *buffer = NULL, *p = NULL;
	char *p = NULL;
	char buffer[20480];
	int fsize = 0;
	STATUS status=FAILED;
	STAT stat;
	char *ext = strrchr(file,'.');
	FILE *fp;
	int move = 0;
	errno = 0;

	fp = fopen(file,"rt");
	if (fp==NULL)
		goto Failed;
	if (FSTAT(fileno(fp),&stat)==0)
	{
		modtime = stat.st_mtime;
		fsize = stat.st_size;
	}
	if(fsize <= 1){
		// empty file short circuit
		return SUCCESS;
	}
	output_verbose("file '%s' is %d bytes long", file,fsize);
	/* removed malloc check since it doesn't malloc any more */
	buffer[0] = '\0';

	move = buffer_read_alt(fp, buffer, file, 20479);
	while(move > 0){
		p = buffer; // grab a block
		while(*p != 0){
			// and process it
			move = gridlabd_file(p);
			if(move == 0)
				break;
			p += move;
		}
		if(*p != 0){
			// failed if we didn't parse the whole thing
			status = FAILED;
			break;
		}
		move = buffer_read_alt(fp, buffer, file, 20479);
	}

	if(p != 0){ /* did the file contain anything? */
		status = (*p=='\0' && !include_fail) ? SUCCESS : FAILED;
	} else {
		status = FAILED;
	}
	if (status==FAILED)
	{
		char *eol = NULL;
		if(p){
			eol = strchr(p,'\n');
		} else {
			p = "";
		}
		if (eol!=NULL){
			*eol='\0';
		}
		output_error_raw("%s(%d): load failed at or near '%.12s...'", file, linenum,*p=='\0'?"end of line":p);
		if (p==0)
			output_error("%s doesn't appear to be a GLM file", file);
		goto Failed;
	}
	else if ((status=load_resolve_all())==FAILED)
		goto Failed;

	/* establish ranks */
	for (obj=first?first:object_get_first(); obj!=NULL; obj=obj->next)
		object_set_parent(obj,obj->parent);
	output_verbose("%d object%s loaded", object_get_count(), object_get_count()>1?"s":"");
	goto Done;
Failed:
	if (errno!=0){
		output_error("unable to load '%s': %s", file, errno?strerror(errno):"(no details)");
		/*	TROUBLESHOOT
			In most cases, strerror(errno) will claim "No such file or directory".  This claim should be ignored in
			favor of prior error messages.
		*/
	}
Done:
	//free(buffer);
	free_index();
	linenum=1; // parser starts at one
	if (fp!=NULL) fclose(fp);
	return status;
}

TECHNOLOGYREADINESSLEVEL calculate_trl(void)
{
	char buffer[1024];
	CLASS *oclass;

	// start optimistically 
	technology_readiness_level = TRL_PROVEN; 
	
	// examine each class loaded
	for ( oclass=class_get_first_class() ; oclass!=NULL ; oclass=oclass->next )
	{
		// if class is inferior
		if ( oclass->profiler.numobjs>0 && oclass->trl<technology_readiness_level )
		{	

			// downgrade trl
			technology_readiness_level = oclass->trl;
			output_verbose("class '%s' TRL is %s", oclass->name, global_getvar("technology_readiness_level",buffer,sizeof(buffer)));
		}
	}
	output_verbose("model TRL is %s", global_getvar("technology_readiness_level",buffer,sizeof(buffer)));
	return technology_readiness_level;
}

/** Load a file
	@return STATUS is SUCCESS if the load was ok, FAILED if there was a problem
	@todo Rollback the model data if the load failed (ticket #32)
	@todo Support nested loads and maintain context during subloads (ticket #33)
 **/
STATUS loadall(char *file){
	char *buffer = NULL, *p = NULL;
	char *ext = file?strrchr(file,'.'):NULL;
	unsigned int old_obj_count = object_get_count();
	unsigned int new_obj_count = 0;
//	unsigned int i;
	char conf[1024];
	static int loaded_files = 0;
	STATUS load_status = FAILED;

	if ( !inline_code_init() ) return FAILED;

	if(old_obj_count > 1 && global_forbid_multiload){
		output_error("loadall: only one file load is supported at this time.");
		return FAILED; /* not what they expected--do not proceed */
	}

	/* first time only */
	if (loaded_files==0)
	{
		/* load the gridlabd.conf file */
		if (find_file("gridlabd.conf",NULL,R_OK,conf,sizeof(conf))==NULL)
			output_warning("gridlabd.conf was not found");
			/* TROUBLESHOOT
				The <code>gridlabd.conf</code> was not found in the <b>GLPATH</b> environment path.
				This file is always loaded before a GLM file is loaded.
				Make sure that <b>GLPATH</b> includes the <code>.../etc</code> folder and try again.
			 */
		else{
			sprintf(filename, "gridlabd.conf");
			if(loadall_glm_roll(conf)==FAILED){
				return FAILED;
			}
		}

		/* load the debugger.conf file */
		if (global_debug_mode)
		{
			char dbg[1024];
			
			if (find_file("debugger.conf",NULL,R_OK,dbg,sizeof(dbg))==NULL)
				output_warning("debugger.conf was not found");
				/* TROUBLESHOOT
					The <code>debugger.conf</code> was not found in the <b>GLPATH</b> environment path.
					This file is loaded when the debugger is enabled.
					Make sure that <b>GLPATH</b> includes the <code>.../etc</code> folder and try again.
				 */
			else if (loadall_glm_roll(dbg)==FAILED)
				return FAILED;
		}
	}

	/* if nothing requested only config files are loaded */
	if ( file==NULL )
		return SUCCESS;

	/* handle default extension */
	strcpy(filename,file);
	if (ext==NULL || ext<file+strlen(file)-5)
	{
		ext = filename+strlen(filename);
		strcat(filename,".glm");
	}

	/* load the appropriate type of file */
	if (global_streaming_io_enabled || (ext!=NULL && isdigit(ext[1])) )
	{
		FILE *fp = fopen(file,"rb");
		if (fp==NULL || stream(fp,SF_IN)<0)
		{
			output_error("%s: unable to read stream", file);
			return FAILED;
		}
		else
			load_status = SUCCESS;
	}
	else if (ext==NULL || strcmp(ext, ".glm")==0)
		load_status = loadall_glm_roll(filename);
#ifdef HAVE_XERCES
	else if(strcmp(ext, ".xml")==0)
		load_status = loadall_xml(filename);
#endif
	else
		output_error("%s: unable to load unknown file type", filename, ext);

	/* objects should not be started until all deferred schedules are done */
	if ( global_threadcount>1 )
	{
		if ( schedule_createwait()==FAILED )
		{
			output_error_raw("%s(%d): load failed on schedule error", filename, linenum);
			return FAILED;
		}
	}

	/* handle new objects */
//	new_obj_count = object_get_count();
//	if((load_status == FAILED) && (old_obj_count < new_obj_count)){
//		for(i = old_obj_count+1; i <= new_obj_count; ++i){
//			object_remove_by_id(i);
//		}
//	}

	calculate_trl();

	/* destroy inline code buffers */
	inline_code_term();

	loaded_files++;
	return load_status;
}

/** @} */

