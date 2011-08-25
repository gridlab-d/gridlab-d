/** $Id$
	Copyright (C) 2008 Battelle Memorial Institute
	@file cmdarg.c
	@addtogroup cmdarg Command-line arguments
	@ingroup core

	The command-line argument processing module processes arguments as they are encountered.

	The following command-line toggles are supported
	- \p --warn		toggles the warning mode
	- \p --check	toggles calls to module check functions
	- \p --debug	toggles debug messages
	- \p --debugger	enables the debugger and turns on debug messages
	- \p --dumpall	toggles a complete model dump when the simulation exits
	- \p --quiet	toggles all messages except \b error and \b fatal messages
	- \p --profile	toggles performance profiling
	- \p --compile  toggles compile-only run mode (model is loaded and saved, but not run)

	The following command-line processes can be called
	- \p --license	prints the software license
	- \p --dsttest	performs a daylight saving time definitions in \b tzinfo.txt
	- \p --unitstest	performs a test of the units in \b unitfile.txt
	- \p --randtest	performs a test of the random number generators
	- \p --scheduletest	performs a test of the built-in schedules
	- \p --loadshapetest	performs a test of the built-in loadshapes 
	- \p --endusetest	performs a test of the built-in enduses
	- \p --testall \e file	performs module selftests of modules those listed in \e file
	- \p --test	run the internal core self-test routines
	- \p --define	define a global variable
	- \p --libinfo \e module	prints information about the \e module
	- \p --xsd \e module[:object]	prints the xsd of a module or object
	- \p --xsl        creates the xsl for this version of gridlab-d
	- \p --kml=file   output kml (Google Earth) file of model

	The following system options may be changed
	- \p --threadcount \e n		changes the number of thread to use during simulation (default is 0, meaning as many as useful)
	- \p --output \e file		saves dump output to \e file (default is \p gridlabd.glm)
	- \p --environment \e app	start the \e app as the processing environment (default is \p batch)
	- \p --xmlencoding \e num	sets the XML encoding (8, 16, or 32)
	- \p --xmlstrict            toggles XML to be strict
	- \p --relax                allows implicit variable definition when assignments made
	- \p --clearmap				clear the processor schedule

	The following are only supported on Linux systems
	- \p --pidfile[=filename]   creates a process id file while GridLAB-D is running (default is gridlabd.pid)
	- \p --redirect \e stream[:file] redirects output stream to file
	- \p --server      runs in server mode (pidfile and redirects all output)
 @{
 **/

#include <stdio.h>
#include <string.h>

#include "platform.h"
#include "globals.h"
#include "cmdarg.h"
#include "output.h"
#include "load.h"
#include "legal.h"
#include "timestamp.h"
#include "random.h"
#include "loadshape.h"
#include "enduse.h"

clock_t loader_time = 0;

STATUS load_module_list(FILE *fd,int* test_mod_num)
{
	/*

	sprintf(mod_test,"mod_test%d=%s",test_mod_num++,*++argv);
	if (global_setvar(mod_test)==SUCCESS)
	*/
	char mod_test[100];
	char line[100];
	while(fscanf(fd,"%s",line) != EOF)
	{
		printf("Line: %s",line);
		sprintf(mod_test,"mod_test%d=%s",(*test_mod_num)++,line);
		if (global_setvar(mod_test)!=SUCCESS)
		{
			output_fatal("Unable to store module name");
			/*	TROUBLESHOOT
				This error is caused by a failure to set up a module test, which
				requires that the name module being tested be stored in a global
				variable called mod_test<num>.  The root cause will be identified
				by determining what error in the global_setvar call occurred.
			 */
			return FAILED;
		}
	}

	return SUCCESS;
}

typedef struct s_pntree{
	char *name;
	CLASS *oclass;
	struct s_pntree *left, *right;
} pntree;

void modhelp_alpha(pntree **ctree, CLASS *oclass){
	int cmpval = 0;
	pntree *targ = *ctree;
	
	cmpval = strcmp(oclass->name, targ->name);
	
	if(cmpval == 0){
		; /* exception? */
	} if(cmpval < 0){ /*  class < root ~ go left */
		if(targ->left == NULL){
			targ->left = (pntree *)malloc(sizeof(pntree));
			memset(targ->left, 0, sizeof(pntree));
			targ->left->name = oclass->name;
			targ->left->name = oclass->name;
			targ->left->oclass = oclass;
			targ->left->left = targ->left->right = 0;
		} else { /* non-null, follow upwards */
			modhelp_alpha(&targ->left, oclass);
		}
	} else {
		if(targ->right == NULL){
			targ->right = (pntree *)malloc(sizeof(pntree));
			memset(targ->right, 0, sizeof(pntree));
			targ->right->name = oclass->name;
			targ->right->name = oclass->name;
			targ->right->oclass = oclass;
			targ->right->right = targ->right->left = 0;
		} else {
			modhelp_alpha(&targ->right, oclass);
		}
	}
}

void set_tabs(char *tabs, int tabdepth){
	if(tabdepth > 32){
		throw_exception("print_class_d: tabdepth > 32, which is mightily deep!");
		/* TROUBLESHOOT
			This means that there is very deep nesting and this is unexpected.  
			This suggests a problem with the internal model and should be reported.
		 */
	} else {
		int i = 0;
		memset(tabs, 0, 33);
		for(i = 0; i < tabdepth; ++i)
			tabs[i] = '\t';
	}
}

void print_class_d(CLASS *oclass, int tabdepth){
	PROPERTY *prop;
	FUNCTION *func;
	char tabs[33];

	set_tabs(tabs, tabdepth);

	printf("%sclass %s {\n", tabs, oclass->name);
	if (oclass->parent){
		printf("%s\tparent %s;\n", tabs, oclass->parent->name);
		print_class_d(oclass->parent, tabdepth+1);
	}
	for (func=oclass->fmap; func!=NULL && func->oclass==oclass; func=func->next)
		printf( "%s\tfunction %s();\n", tabs, func->name);
	for (prop=oclass->pmap; prop!=NULL && prop->oclass==oclass; prop=prop->next)
	{
		char *propname = class_get_property_typename(prop->ptype);
		if (propname!=NULL){
			if(prop->unit != NULL)
			{
				printf("%s\t%s %s[%s];", tabs, propname, prop->name, prop->unit->name);
			}
			else if (prop->ptype==PT_set || prop->ptype==PT_enumeration)
			{
				KEYWORD *key;
				printf("%s\t%s {", tabs, propname);
				for (key=prop->keywords; key!=NULL; key=key->next)
					printf("%s=%"FMT_INT64"u%s", key->name, (int64)key->value, key->next==NULL?"":", ");
				printf("} %s;", prop->name);
			} 
			else 
			{
				printf("%s\t%s %s;", tabs, propname, prop->name);
			}
			if (prop->description!=NULL)
				printf(" // %s%s",prop->flags&PF_DEPRECATED?"(DEPRECATED) ":"",prop->description);
			printf("\n");
		}
	}
	printf("%s}\n\n", tabs);
}

void print_class(CLASS *oclass){
	print_class_d(oclass, 0);
}

void print_modhelp_tree(pntree *ctree){
	if(ctree->left != NULL){
		print_modhelp_tree(ctree->left);
		free(ctree->left);
		ctree->left = 0;
	}
	print_class(ctree->oclass);
	if(ctree->right != NULL){
		print_modhelp_tree(ctree->right);
		free(ctree->right);
		ctree->right = 0;
	}
}

int compare(const void *a, const void *b)
{
	return stricmp(*(char**)a,*(char**)b);
}

typedef struct s_cmdarg {
	char *lopt;
	char *sopt;
	int (*call)(int argc, char *argv[]);
	char *args;
	char *desc;
} CMDARG;
static int help(int argc, char *argv[]);

/************************************************************************/
/* COMMAND LINE PARSING ROUTINES 
 *
 * All cmdline parsing routines must use the prototype int (*)(int,char *[])
 *
 * The return value must be the number of args processed (excluding primary 
 * one). A return value of -1 indicates that processing must stop immediately.
 *
 */
static int copyright(int argc, char *argv[])
{
	legal_notice();
	return 0;
}
static int warn(int argc, char *argv[])
{
	global_warn_mode = !global_warn_mode;
	return 0;
}
static int bothstdout(int argc, char *argv[])
{
	output_both_stdout();
	return 0;
}
static int check(int argc, char *argv[])
{
	global_runchecks = !global_runchecks;
	return 0;
}
static int debug(int argc, char *argv[])
{
	global_debug_output = !global_debug_output;
	return 0;
}
static int debugger(int argc, char *argv[])
{
	global_debug_mode = 1;
	global_debug_output = !global_debug_output;
	return 0;
}
static int dumpall(int argc, char *argv[])
{
	global_dumpall = !global_dumpall;
	return 0;
}
static int quiet(int argc, char *argv[])
{
	global_quiet_mode = !global_quiet_mode;
	return 0;
}
static int verbose(int argc, char *argv[])
{
	global_verbose_mode=!global_verbose_mode;
	return 0;
}
static int _check_version(int argc, char *argv[])
{
	check_version(0);
	return 0;
}
static int profile(int argc, char *argv[])
{
	global_profiler = !global_profiler;
	return 0;
}
static int pause(int argc, char *argv[])
{
	global_pauseatexit = !global_pauseatexit;
	return 0;
}
static int compile(int argc, char *argv[])
{
	global_compileonly = !global_compileonly;
	return 0;
}
static int license(int argc, char *argv[])
{
	legal_license();
	return 0;
}
static int server_portnum(int argc, char *argv[])
{
	if (argc>1)
	{
		global_server_portnum = (argc--,atoi(*++argv));
		return 1;
	}
	else
	{
		output_fatal("missing server port number");
		/*	TROUBLESHOOT
			The <b>-P</b> or <b>--server_portnum</b> command line directive
			was not followed by a valid number.  The correct syntax is
			<b>-P <i>number</i></b> or <b>--server_portnum <i>number</i></b>.
		 */
		return -1;
	}
}
static int version(int argc, char *argv[])
{
#ifdef PACKAGE_STRING
	output_message("%s %s",PACKAGE_STRING, BRANCH);
#else
#ifdef I64
#define ARCH "WIN64"
#else
#define ARCH "WIN32" 
#endif
#ifdef _DEBUG
#define REL "DEBUG"
#else
#define REL "RELEASE"
#endif
	int buildnum;
	if (sscanf(BUILD,"$%*[^:]: %d",&buildnum)==1 && buildnum>0)
		output_message("GridLAB-D %d.%d.%d.%d %s",REV_MAJOR,REV_MINOR,REV_PATCH,buildnum,BRANCH);
	else	
		output_message("GridLAB-D %d.%d.%d.%s-%s %s",REV_MAJOR,REV_MINOR,REV_PATCH,ARCH,REL,BRANCH);
#endif
	return 0;
}
static int dsttest(int argc, char *argv[])
{
	timestamp_test();
	return 0;
}
static int randtest(int argc, char *argv[])
{
	random_test();
	return 0;
}
static int unitstest(int argc, char *argv[])
{
	unit_test();
	return 0;
}
static int scheduletest(int argc, char *argv[])
{
	schedule_test();
	return 0;
}
static int loadshapetest(int argc, char *argv[])
{
	loadshape_test();
	return 0;
}
static int endusetest(int argc, char *argv[])
{
	enduse_test();
	return 0;
}
static int xmlstrict(int argc, char *argv[])
{
	global_xmlstrict = !global_xmlstrict;
	output_verbose("xmlstrict is %s", global_xmlstrict?"enabled":"disabled");
	return 0;
}
static int globaldump(int argc, char *argv[])
{
	global_dump();
	return -1;
}
static int relax(int argc, char *argv[])
{
	global_strictnames = FALSE;
	return 0;
}
static int pidfile(int argc, char *argv[])
{
	char *filename = strchr(*argv,'=');
	if (filename==NULL)
		strcpy(global_pidfile,"gridlabd.pid");
	else
		strcpy(global_pidfile,filename+1);
	return 0;
}
static int kml(int argc, char *argv[])
{
	char *filename = strchr(*argv,'=');
	if (filename)
		strcpy(global_kmlfile,filename+1);
	else
		strcpy(global_kmlfile,"gridlabd.kml");
	return 0;
}
static int avlbalance(int argc, char *argv[])
{
	global_no_balance = !global_no_balance;
	return 0;
}
static int testall(int argc, char *argv[])
{
	int test_mod_num = 1;
	FILE *fd = NULL;
	if(*++argv != NULL)
		fd = fopen(*argv,"r");
	else {
		output_fatal("no filename for testall");
		/*	TROUBLESHOOT
			The --testall parameter was found on the command line, but
			if was not followed by a filename containing the test
			description file.
		*/
		return -1;
	}
	argc--;
	global_test_mode=TRUE;

	if(fd == NULL)
	{
		output_fatal("incorrect module list file name");
		/*	TROUBLESHOOT
			The --testall parameter was found on the command line, but
			if was not followed by a valid filename containing the test
			description file.
		*/
		return -1;
	}
	if(load_module_list(fd,&test_mod_num) == FAILED)
		return -1;
	return 1;
}
static int modhelp(int argc, char *argv[])
{
	if(argc > 1){
		MODULE *mod = NULL;
		CLASS *oclass = NULL;
		argv++;
		argc--;
		if(strchr(argv[0], ':') == 0){ // no class
			mod = module_load(argv[0],0,NULL);
		} else {
			GLOBALVAR *var=NULL;
			char *cname;
			cname = strchr(argv[0], ':')+1;
			mod = module_load(strtok(argv[0],":"),0,NULL);
			oclass = class_get_class_from_classname(cname);
			if(oclass == NULL){
				output_fatal("Unable to find class '%s' in module '%s'", cname, argv[0]);
				/*	TROUBLESHOOT
					The <b>--modhelp</b> parameter was found on the command line, but
					if was followed by a class specification that isn't valid.
					Verify that the class exists in the module you specified.
				*/
				return FAILED;
			}

			/* dump module globals */
			printf("module %s {\n", mod->name);
			while ((var=global_getnext(var))!=NULL)
			{
				PROPERTY *prop = var->prop;
				char *proptype = class_get_property_typename(prop->ptype);
				if (strncmp(var->prop->name,mod->name,strlen(mod->name))!=0)
					continue;
				if (proptype!=NULL){
					if(prop->unit != NULL)
					{
						printf("\t%s %s[%s];", proptype, strrchr(prop->name,':')+1, prop->unit->name);
					}
					else if (prop->ptype==PT_set || prop->ptype==PT_enumeration)
					{
						KEYWORD *key;
						printf("\t%s {", proptype);
						for (key=prop->keywords; key!=NULL; key=key->next)
							printf("%s=%"FMT_INT64"d%s", key->name, key->value, key->next==NULL?"":", ");
						printf("} %s;", strrchr(prop->name,':')+1);
					} 
					else 
					{
						printf("\t%s %s;", proptype, strrchr(prop->name,':')+1);
					}
					if (prop->description!=NULL)
						printf(" // %s%s",prop->flags&PF_DEPRECATED?"(DEPRECATED) ":"",prop->description);
					printf("\n");
				}
			}
			printf("}\n");
		}
		if(mod == NULL){
			output_fatal("module %s is not found",*argv);
			/*	TROUBLESHOOT
				The <b>--modhelp</b> parameter was found on the command line, but
				if was followed by a module specification that isn't valid.
				Verify that the module exists in GridLAB-D's <b>lib</b> folder.
			*/
			return FAILED;
		}
		if(oclass != NULL)
		{
			print_class(oclass);
		}
		else
		{
			CLASS	*oclass;
			pntree	*ctree;
			/* lexographically sort all elements from class_get_first_class & oclass->next */

			oclass=class_get_first_class();
			ctree = (pntree *)malloc(sizeof(pntree));
			
			if(ctree == NULL){
				throw_exception("--modhelp: malloc failure");
				/* TROUBLESHOOT
					The memory allocation needed for module help to function has failed.  Try freeing up system memory and try again.
				 */
			}
			
			ctree->name = oclass->name;
			ctree->oclass = oclass;
			ctree->left = ctree->right = 0;
			
			for(; oclass != NULL; oclass = oclass->next){
				modhelp_alpha(&ctree, oclass);
				//print_class(oclass);
			}

			/* flatten tree */
			print_modhelp_tree(ctree);
		}
	}
	return 1;
}
static int modtest(int argc, char *argv[])
{
	if (argc>1)
	{
		MODULE *mod = module_load(argv[1],0,NULL);
		if (mod==NULL)
			output_fatal("module %s is not found",argv[1]);
			/*	TROUBLESHOOT
				The <b>--modtest</b> parameter was found on the command line, but
				if was followed by a module specification that isn't valid.
				Verify that the module exists in GridLAB-D's <b>lib</b> folder.
			*/
		else 
		{
			argv++;argc--;
			if (mod->test==NULL)
				output_fatal("module %s does not implement a test routine", argv[0]);
				/*	TROUBLESHOOT
					The <b>--modtest</b> parameter was found on the command line, but
					if was followed by a specification for a module that doesn't
					implement any test procedures.  See the <b>--libinfo</b> command
					line parameter for information on which procedures the
					module supports.
				*/
			else
			{
				output_test("*** modtest of %s beginning ***", argv[0]);
				mod->test(0,NULL);
				output_test("*** modtest of %s ended ***", argv[0]);
			}
		}			
	}
	else
	{
		output_fatal("definition is missing");
		/*	TROUBLESHOOT
			The <b>--modtest</b> parameter was found on the command line, but
			if was not followed by a module specification.  The correct
			syntax is <b>gridlabd --modtest <i>module_name</i></b>.
		*/
		return FAILED;
	}
	return 1;
}
static int test(int argc, char *argv[])
{
	int test_mod_num = 1;
	global_test_mode=TRUE;
	global_strictnames = FALSE;
	output_debug("disabling strict naming for tests");
	if (argc>1)
	{
		char mod_test[100];
		sprintf(mod_test,"mod_test%d=%s",test_mod_num++,*++argv);
		if (global_setvar(mod_test)==SUCCESS)
			argc--;
	}
	else
	{
		output_fatal("test module name is missing");
		/*	TROUBLESHOOT
			The <b>--test</b> parameter was found on the command line, but
			if was not followed by a module specification that is valid.
			The correct syntax is <b>gridlabd --test <i>module_name</i></b>.
		*/
		return -1;
	}
	return 1;
}
static int define(int argc, char *argv[])
{
	if (argc>1)
	{
		bool namestate = global_strictnames;
		global_strictnames = FALSE;
		if (global_setvar(*++argv,NULL)==SUCCESS){
			argc--;
		}
		global_strictnames = namestate;
		return 1;
	}
	else
	{
		output_fatal("definition is missing");
		/* TROUBLESHOOT
			The <b>-D</b> or <b>--define</b> command line parameters was given, but
			it was not followed by a variable definition.  The correct syntax
			<b>-D </i>variable</i>=<i>value</i></b> or
			<b>--define </i>variable</i>=<i>value</i></b>
		 */
		return -1;
	}
}
static int globals(int argc, char *argv[])
{
	char *list[65536];
	int i, n=0;
	GLOBALVAR *var = NULL;

	/* load the list into the array */
	while ((var=global_getnext(var))!=NULL)
	{
		if (n<sizeof(list)/sizeof(list[0]))
			list[n++] = var->prop->name;
		else
		{
			output_fatal("--globals has insufficient buffer space to sort globals list");
			return FAILED;
		}
	}

	/* sort the array */
	qsort(list,n,sizeof(list[0]),compare);

	/* output sorted array */
	for (i=0; i<n; i++)
	{
		char buffer[1024];
		var = global_find(list[i]);
		printf("%s=%s;",var->prop->name,global_getvar(var->prop->name,buffer,sizeof(buffer))?buffer:"(error)");
		if (var->prop->description || var->prop->flags&PF_DEPRECATED)
			printf(" // %s%s", (var->prop->flags&PF_DEPRECATED)?"DEPRECATED ":"", var->prop->description?var->prop->description:"");
		printf("\n");
	}
	return 0;
}
static int redirect(int argc, char *argv[])
{
	if (argc>1)
	{
		char buffer[1024]; char *p;
		strcpy(buffer,*++argv); argc--;
		if (strcmp(buffer,"all")==0)
		{
			if (output_redirect("output",NULL)==NULL ||
				output_redirect("error",NULL)==NULL ||
				output_redirect("warning",NULL)==NULL ||
				output_redirect("debug",NULL)==NULL ||
				output_redirect("verbose",NULL)==NULL ||
				output_redirect("profile",NULL)==NULL ||
				output_redirect("progress",NULL)==NULL)
			{
				output_fatal("redirection of all failed");
				/* TROUBLESHOOT
					An attempt to close all standard stream from the
					command line using <b>--redirect all</b> has failed.
					One of the streams cannot be closed.  Try redirecting
					each stream separately until the problem stream is
					identified and the correct the problem with that stream.
				 */
				return FAILED;
			}
		}
		else if ((p=strchr(buffer,':'))!=NULL)
		{
			*p++='\0';
			if (output_redirect(buffer,p)==NULL)
			{
				output_fatal("redirection of %s to '%s' failed: %s",buffer,p, strerror(errno));
				/*	TROUBLESHOOT
					An attempt to redirect a standard stream from the 
					command line using <b>--redirect <i>stream</i>:<i>destination</i></b>
					has failed.  The message should provide an indication of why the
					attempt failed. The remedy will depend on the nature of the problem.
				 */
				return FAILED;
			}
		}
		else if (output_redirect(buffer,NULL)==NULL)
		{
				output_fatal("default redirection of %s failed: %s",buffer, strerror(errno));
				/*	TROUBLESHOOT
					An attempt to close a standard stream from the 
					command line using <b>--redirect <i>stream</i></b>
					has failed.  The message should provide an indication of why the
					attempt failed. The remedy will depend on the nature of the problem.
					
				 */
				return FAILED;
		}
	}
	else
	{
		output_fatal("redirection is missing");
		/*	TROUBLESHOOT
			A <b>--redirect</b> directive on the command line is missing
			its redirection specification.  The correct syntax is
			<b>--redirect <i>stream</i>[:<i>destination</i>]</b>.
		 */
		return FAILED;
	}
	return 1;
}
static int libinfo(int argc, char *argv[])
{
	if (argc-1>0)
	{	argc--;
		module_libinfo(*++argv);
	}
	else
	{
		output_fatal("missing library name");
		/*	TROUBLESHOOT
			The <b>-L</b> or <b>--libinfo</b> command line directive
			was not followed by a module name.  The correct syntax is
			<b>-L <i>module_name</i></b> or <b>--libinfo <i>module_name</i></b>.
		 */
	}
	return -1;
}
static int threadcount(int argc, char *argv[])
{
	if (argc>1)
		global_threadcount = (argc--,atoi(*++argv));
	else
	{
		output_fatal("missing thread count");
		/*	TROUBLESHOOT
			The <b>-T</b> or <b>--threadcount</b> command line directive
			was not followed by a valid number.  The correct syntax is
			<b>-T <i>number</i></b> or <b>--threadcount <i>number</i></b>.
		 */
		return -1;
	}
	return 1;
}
static int output(int argc, char *argv[])
{
	if (argc>1)
		strcpy(global_savefile,(argc--,*++argv));
	else
	{
		output_fatal("missing output file");
		/* TROUBLESHOOT
			The <b>-o</b> or <b>--output</b> command line directive
			was not followed by a valid filename.  The correct syntax is
			<b>-o <i>file</i></b> or <b>--output <i>file</i></b>.
		 */
		return -1;
	}
	return 1;
}
static int environment(int argc, char *argv[])
{
	if (argc>1)
		strcpy(global_environment,(argc--,*++argv));
	else
	{
		output_fatal("environment not specified");
		/*	TROUBLESHOOT
			The <b>-e</b> or <b>--environment</b> command line directive
			was not followed by a valid environment specification.  The
			correct syntax is <b>-e <i>keyword</i></b> or <b>--environment <i>keyword</i></b>.
		 */
		return -1;
	}
	return 1;
}
static int xmlencoding(int argc, char *argv[])
{
	if (argc>1)
	{
		global_xml_encoding = atoi(*++argv);
		argc--;
	}
	else
	{
		output_fatal("xml encoding not specified");
		/*	TROUBLESHOOT
			The <b>--xmlencoding</b> command line directive
			was not followed by a encoding specification.  The
			correct syntax is <b>--xmlencoding <i>keyword</i></b>.
		 */
		return FAILED;
	}
	return 1;
}
static int xsd(int argc, char *argv[])
{
	if (argc>0)
	{
		argc--;
		output_xsd(*++argv);
		return -1;
	}
	else
	{
		MODULE *mod;
		for (mod=module_get_first(); mod!=NULL; mod=mod->next)
			output_xsd(mod->name);
		return 0;
	}
}
static int xsl(int argc, char *argv[])
{
	if (argc-1>0)
	{
		char fname[1024];
		char *p_arg = *++argv;
		char n_args=1;
		char **p_args;
		argc--;
		while (*p_arg++!='\0') if (*p_arg==',')	n_args++;
		p_args = (char**)malloc(sizeof(char*)*n_args);
		p_arg = strtok(*argv,",");
		n_args=0;
		while (p_arg!=NULL)
		{
			p_args[n_args++] = p_arg;
			p_arg = strtok(NULL,",");
		}
		sprintf(fname,"gridlabd-%d_%d.xsl",global_version_major,global_version_minor);
		output_xsl(fname,n_args,p_args);
		return -1;
	}
	else
	{
		output_fatal("module list not specified");
		/*	TROUBLESHOOT
			The <b>--xsl</b> command line directive
			was not followed by a validlist of modules.  The
			correct syntax is <b>--xsl <i>module1</i>[,<i>module2</i>[,...]]</b>.
		 */
		return -1;
	}
}
static int stream(int argc, char *argv[])
{
	global_streaming_io_enabled = !global_streaming_io_enabled;
	return 0;
}
static int server(int argc, char *argv[])
{
	strcpy(global_environment,"server");
	return 0;
}
static int clearmap(int argc, char *argv[])
{
	sched_clear();
	return 0;
}
static int pstatus(int argc, char *argv[])
{
	sched_print();
	return 0;
}

/*********************************************/
/* ADD NEW CMDARG PROCESSORS ABOVE THIS HERE */
/* Then make the appropriate entry in the    */
/* CMDARG structure below                    */
/*********************************************/

static CMDARG main[] = {

	/* NULL,NULL,NULL,NULL, "Section heading */
	{NULL,NULL,NULL,NULL, "Command-line options"},

	/* long_str, short_str, processor_call, arglist_desc, help_desc */
	{"check",		"c",	check,			NULL, "Performs module checks before starting simulation" },
	{"debug",		NULL,	debug,			NULL, "Toggles display of debug messages" },
	{"debugger",	NULL,	debugger,		NULL, "Enables the debugger" },
	{"dumpall",		NULL,	dumpall,		NULL, "Dumps the global variable list" },
	{"quiet",		"q",	quiet,			NULL, "Toggles suppression of all but error and fatal messages" },
	{"profile",		NULL,	profile,		NULL, "Toggles performance profiling of core and modules while simulation run" },
	{"verbose",		"v",	verbose,		NULL, "Toggles output of verbose messages" },
	{"warn",		"w",	warn,			NULL, "Toggles display of warning messages" },
	
	{NULL,NULL,NULL,NULL, "Global and module control"},
	{"define",		"D",	define,			"<name>=[<module>:]<value>", "Defines or sets a global (or module) variable" },
	{"globals",		NULL,	globals,		NULL, "Displays a sorted list of all global variables" },
	{"libinfo",		"L",	libinfo,		"<module>", "Displays information about a module" },

	{NULL,NULL,NULL,NULL, "Information"},
	{"copyright",	NULL,	copyright,		NULL, "Displays copyright" },
	{"license",		NULL,	license,		NULL, "Displays the license agreement" },
	{"version",		"V",	version,		NULL, "Displays the version information" },

	{NULL,NULL,NULL,NULL, "Test processes"},
	{"dsttest",		NULL,	dsttest,		NULL, "Perform daylight savings rule test" },
	{"endusetest",	NULL,	endusetest,		NULL, "Perform enduse pseudo-object test" },
	{"globaldump",	NULL,	globaldump,		NULL, "Perform a dump of the global variables" },
	{"loadshapetest", NULL,	loadshapetest,	NULL, "Perform loadshape pseudo-object test" },
	{"modtest",		NULL,	modtest,		"<module>", "Perform test function provided by module" },
	{"randtest",	NULL,	randtest,		NULL, "Perform random number generator test" },
	{"scheduletest", NULL,	scheduletest,	NULL, "Perform schedule pseudo-object test" },	
	{"unitstest",	NULL,	unitstest,		NULL, "Perform unit conversion system test" },
	{"test",		NULL,	test,			"<module>", "Perform unit test of module (deprecated)" },
	{"testall",		NULL,	testall,		"=<filename>", "Perform tests of modules listed in file" },

	{NULL,NULL,NULL,NULL, "File and I/O Formatting"},
	{"kml",			NULL,	kml,			"[=<filename>]", "Output to KML (Google Earth) file of model (only supported by some modules)" },
	{"stream",		NULL,	stream,			NULL, "Toggles streaming I/O" },
	{"xmlencoding",	NULL,	xmlencoding,	"8|16|32", "Set the XML encoding system" },
	{"xmlstrict",	NULL,	xmlstrict,		NULL, "Toggle strict XML formatting (default is enabled)" },
	{"xsd",			NULL,	xsd,			"[module[:class]]", "Prints the XSD of a module or class" },
	{"xsl",			NULL,	xsl,			"module[,module[,...]]]", "Create the XSL file for the module(s) listed" },

	{NULL,NULL,NULL,NULL, "Help"},
	{"help",		"h",		help,		NULL, "Displays command line help" },
	{"modhelp",		NULL,		modhelp,	"module[:class]", "Display structure of a class or all classes in a module" },

	{NULL,NULL,NULL,NULL, "Process control"},
	{"pidfile",		NULL,	pidfile,		"[=<filename>]", "Set the process ID file (default is gridlabd.pid)" },
	{"threadcount", "T",	threadcount,	"<n>", "Set the maximum number of threads allowed" },

	{NULL,NULL,NULL,NULL, "System options"},
	{"avlbalance",	NULL,	avlbalance,		NULL, "Toggles automatic balancing of object index" },
	{"bothstdout",	NULL,	bothstdout,		NULL, "Merges all output on stdout" },
	{"check_version", NULL,	_check_version,	NULL, "Perform online version check to see if any updates are available" },
	{"compile",		"C",	compile,		NULL, "Toggles compile-only flags" },
	{"environment",	"e",	environment,	"<appname>", "Set the application to use for run environment" },
	{"output",		"o",	output,			"<file>", "Enables save of output to a file (default is gridlabd.glm)" },
	{"pause",		NULL,	pause,			NULL, "Toggles pause-at-exit feature" },
	{"relax",		NULL,	relax,			NULL, "Allows implicit variable definition when assignments are made" },

	{NULL,NULL,NULL,NULL, "Server mode"},
	{"server",		NULL,	server,			NULL, "Enables the server"},
	{"clearmap",	NULL,	clearmap,		NULL, "Clears the process map" },
	{"pstatus",		NULL,	pstatus,		NULL, "Prints the process list" },
	{"redirect",	NULL,	redirect,		"<stream>[:<file>]", "Redirects an output to stream to a file (or null)" },
	{"server_portnum", "P", server_portnum, NULL, "Sets the server port number (default is 6267)" },
};

static int help(int argc, char *argv[])
{
	int i;
	int old = global_suppress_repeat_messages;
	size_t indent = 0;
	global_suppress_repeat_messages = 0;
	output_message("Syntax: gridlabd [<options>] file1 [file2 [...]]");

	for ( i=0 ; i<sizeof(main)/sizeof(main[0]) ; i++ )
	{
		CMDARG arg = main[i];
		size_t len = (arg.sopt?strlen(arg.sopt):0) + (arg.lopt?strlen(arg.lopt):0) + (arg.args?strlen(arg.args):0);
		if (len>indent) indent = len;
	}

	for ( i=0 ; i<sizeof(main)/sizeof(main[0]) ; i++ )
	{
		CMDARG arg = main[i];

		/* if this entry is a heading */
		if ( arg.lopt==NULL && arg.sopt==NULL && arg.call==NULL && arg.args==NULL)
		{
			size_t l = strlen(arg.desc);
			char buffer[1024];
			memset(buffer,0,1024);
			while ( l-->0 )
				buffer[l] = '-';
			output_message("");
			output_message("%s", arg.desc);
			output_message("%s", buffer);
		}
		else 
		{
			char buffer[1024] = "  ";
			if ( arg.lopt ) 
			{
				strcat(buffer,"--");
				strcat(buffer,arg.lopt);
			}
			if ( arg.sopt )
			{
				if ( arg.lopt )
					strcat(buffer,"|");
				strcat(buffer,"-");
				strcat(buffer,arg.sopt);
			}
			if ( arg.args==NULL || ( arg.args[0]!='=' && strncmp(arg.args,"[=",2)!=0 ) )
				strcat(buffer," ");
			if ( arg.args )
			{
				strcat(buffer,arg.args);
				strcat(buffer," ");
			}
			while ( strlen(buffer) < indent+8 )
				strcat(buffer," ");
			strcat(buffer,arg.desc);
			output_message("%s", buffer);
		}
	}
	global_suppress_repeat_messages = old;
	return 0;
}

/** Load and process the command-line arguments
	@return a STATUS value

	Arguments are processed immediately as they are seen.  This means that
	models are loaded when they are encountered, and in relation to the
	other flags.  Thus
	@code
	gridlabd --warn model1 --warn model2
	@endcode
	will load \p model1 with warnings on, and \p model2 with warnings off.
 **/
STATUS cmdarg_load(int argc, /**< the number of arguments in \p argv */
				   char *argv[]) /**< a list pointers to the argument string */
{
	STATUS status = SUCCESS;

	while (argv++,--argc>0)
	{
		int found = 0;
		int i;
		for ( i=0 ; i<sizeof(main)/sizeof(main[0]) ; i++ )
		{
			CMDARG arg = main[i];
			if ( ( arg.sopt && strncmp(*argv,"-",1)==0 && strcmp((*argv)+1,arg.sopt)==0 ) ||
				 ( arg.lopt && strncmp(*argv,"--",2)==0 && strcmp((*argv)+2,arg.lopt)==0 ) )
			{
				int n = arg.call(argc,argv);
				if ( n==-1 )
					return status;
				found = 1;
				argc -= n;
				argv += n;
				break;
			}
		}

		/* cmdarg not processed */
		if ( !found )
		{
			/* file name */
			if (**argv!='-')
			{
				if (global_test_mode)
					output_warning("file '%s' ignored in test mode", *argv);
					/* TROUBLESHOOT
					   This warning is caused by an attempt to read an input file in self-test mode.  
					   The use of self-test model precludes reading model files.  Try running the system
					   in normal more or leaving off the model file name.
					 */
				else {
					clock_t start = clock();

					if (!loadall(*argv))
						status = FAILED;
					loader_time += clock() - start;

					/* preserve name of first model only */
					if (strcmp(global_modelname,"")==0)
						strcpy(global_modelname,*argv);
				}
			}

			/* send cmdarg to modules */
			else
			{
				int n = module_cmdargs(argc,argv);
				if (n==0)
				{
					output_error("command line option '%s' is not recognized",*argv);
					/* TROUBLESHOOT
						The command line option given is not valid where it was found.
						Check the command line for correct syntax and order of options.
					 */
					status = FAILED;
				}
			}			
		}
	}
	return status;
}

/**@}**/
