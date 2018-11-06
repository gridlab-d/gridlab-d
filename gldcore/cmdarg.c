/** $Id: cmdarg.c 5518 2016-07-15 00:55:28Z andyfisher $
	Copyright (C) 2008 Battelle Memorial Institute
	@file cmdarg.c
	@addtogroup cmdarg Command-line arguments
	@ingroup core

	The command-line argument processing module processes arguments as they are encountered.
	
	Use the --help command line option to obtain a list of valid options.

 @{
 **/

#include <stdio.h>
#include <string.h>
#ifdef WIN32 && !(__MINGW__)
#include <direct.h>
#else
#include <unistd.h>
#endif

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
#include "instance.h"
#include "test.h"
#include "setup.h"
#include "sanitize.h"
#include "exec.h"
#include "daemon.h"

SET_MYCONTEXT(DMC_CMDARG)

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
		if (propname!=NULL)
		{
			if ( (prop->access&PA_HIDDEN)==PA_HIDDEN )
				continue;
			if (prop->unit != NULL)
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
 * one). A return value of CMDOK indicates that processing must stop immediately
 * and the return status is the current status.  A return value of CMDERR indicates
 * that processing must stop immediately and FAILED status is returned.
 *
 */

static STATUS no_cmdargs()
{
	char htmlfile[1024];
	if ( global_autostartgui && find_file("gridlabd.htm",NULL,R_OK,htmlfile,sizeof(htmlfile)-1)!=NULL )
	{
		char cmd[1024];

		/* enter server mode and wait */
#ifdef WIN32
		if ( htmlfile[1]!=':' )
			sprintf(htmlfile,"%s\\gridlabd.htm", global_workdir);
		output_message("opening html page '%s'", htmlfile);
		sprintf(cmd,"start %s file:///%s", global_browser, htmlfile);
#elif defined(MACOSX)
		sprintf(cmd,"open -a %s %s", global_browser, htmlfile);
#else
		sprintf(cmd,"%s '%s' & ps -p $! >/dev/null", global_browser, htmlfile);
#endif
		IN_MYCONTEXT output_verbose("Starting browser using command [%s]", cmd);
		if (system(cmd)!=0)
		{
			output_error("unable to start browser");
			return FAILED;
		}
		else
		{
			IN_MYCONTEXT output_verbose("starting interface");
		}
		strcpy(global_environment,"server");
		global_mainloopstate = MLS_PAUSED;
		return SUCCESS;
	}
	else
		output_error("default html file '%s' not found (workdir='%s')", "gridlabd.htm",global_workdir);

	return SUCCESS;
}

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
	/* check main core implementation */
	if ( property_check()==FAILED )
	{
		output_fatal("main core property implementation failed size checks");
		exit(XC_INIERR);
	}
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
	global_debug_output = 1;
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
static int mt_profile(int argc, char *argv[])
{
	if ( argc>1 )
	{
		global_profiler = 1;
		global_mt_analysis = atoi(*++argv);
		argc--;
		if ( global_threadcount>1 )
			output_warning("--mt_profile forces threadcount=1");
		if ( global_mt_analysis<2 )
		{
			output_error("--mt_profile <n-threads> value must be 2 or greater");
			return CMDERR;
		}
		else
		{
			global_threadcount = 1;
			return 1;
		}
	}
	else
	{
		output_fatal("missing mt_profile thread count");
		return CMDERR;
	}
}
static int pauseatexit(int argc, char *argv[])
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
		return CMDERR;
	}
}
static int server_inaddr(int argc, char *argv[])
{
	if ( argc>1 )
	{
		strncpy(global_server_inaddr,(argc--,*++argv),sizeof(global_server_inaddr)-1);
		return 1;
	}
	else
	{
		output_fatal("missing server ip address");
		/*	TROUBLESHOOT
			The <b>--server_inaddr</b> command line directive
			was not followed by a valid IP address.  The correct syntax is
			<b>--server_inaddr <i>interface address</i></b>.
		 */
		return CMDERR;
	}
}
static int version(int argc, char *argv[])
{
	output_message("GridLAB-D %d.%d.%d-%d (%s) %d-bit %s %s", 
		global_version_major, global_version_minor, global_version_patch, 
		global_version_build, global_version_branch, 8*sizeof(void*), global_platform,
#ifdef _DEBUG
	"DEBUG"
#else
	"RELEASE"
#endif
	);

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
	IN_MYCONTEXT output_verbose("xmlstrict is %s", global_xmlstrict?"enabled":"disabled");
	return 0;
}
static int globaldump(int argc, char *argv[])
{
	global_dump();
	return CMDOK;
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
		return CMDERR;
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
		return CMDERR;
	}
	if(load_module_list(fd,&test_mod_num) == FAILED)
		return CMDERR;
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
				if ( strncmp(var->prop->name,mod->name,strlen(mod->name))!=0 )
					continue;
				if ( (prop->access&PA_HIDDEN)==PA_HIDDEN )
					continue;
				if (proptype!=NULL)
				{
					if ( prop->unit!=NULL )
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
static int modlist(int arvc, char *argv[])
{
	module_list();
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
	int n=0;
	global_test_mode = TRUE;
	while (argc>1)
	{
		test_request(*++argv);
		argc--;
		n++;
	}
	return n;
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
		return CMDERR;
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
		if ( (var->prop->access&PA_HIDDEN)==PA_HIDDEN )
			continue;
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
		if (strcmp(buffer,"none")==0)
		{} /* used by validate to block default --redirect all behavior */
		else if (strcmp(buffer,"all")==0)
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
		return CMDOK;
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
	return CMDERR;
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
		return CMDERR;
	}
	return 1;
}
static int output(int argc, char *argv[])
{
	if (argc>1)
	{
		strcpy(global_savefile,(argc--,*++argv));
		return 1;
	}
	else
	{
		output_fatal("missing output file");
		/* TROUBLESHOOT
			The <b>-o</b> or <b>--output</b> command line directive
			was not followed by a valid filename.  The correct syntax is
			<b>-o <i>file</i></b> or <b>--output <i>file</i></b>.
		 */
		return CMDERR;
	}
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
		return CMDERR;
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
		return CMDOK;
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
		return CMDOK;
	}
	else
	{
		output_fatal("module list not specified");
		/*	TROUBLESHOOT
			The <b>--xsl</b> command line directive
			was not followed by a validlist of modules.  The
			correct syntax is <b>--xsl <i>module1</i>[,<i>module2</i>[,...]]</b>.
		 */
		return CMDERR;
	}
}
static int _stream(int argc, char *argv[])
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
	sched_init(1);
	sched_print(0);
	return 0;
}
static int pkill(int argc, char *argv[])
{
	if (argc>0)
	{
		argc--;
		sched_pkill(atoi(*++argv));
		return 1;
	}
	else
	{
		output_fatal("processor number not specified");
		/*	TROUBLESHOOT
			The <b>--pkill</b> command line directive
			was not followed by a valid processor number.
			The correct syntax is <b>--pkill <i>processor_number</i></b>.
		 */
		return CMDERR;
	}
}
static int plist(int argc, char *argv[])
{
	sched_init(1);
	sched_print(0);
	return 0;
}
static int pcontrol(int argc, char *argv[])
{
	sched_init(1);
	sched_controller();
	return 0;
}
static int info(int argc, char *argv[])
{
	if ( argc>1 )
	{
		char cmd[1024];
#ifdef WIN32
		sprintf(cmd,"start %s \"%s%s\"", global_browser, global_infourl, argv[1]);
#elif defined(MACOSX)
		sprintf(cmd,"open -a %s \"%s%s\"", global_browser, global_infourl, argv[1]);
#else
		sprintf(cmd,"%s \"%s%s\" & ps -p $! >/dev/null", global_browser, global_infourl, argv[1]);
#endif
		IN_MYCONTEXT output_verbose("Starting browser using command [%s]", cmd);
		if (system(cmd)!=0)
		{
			output_error("unable to start browser");
			return CMDERR;
		}
		else
		{
			IN_MYCONTEXT output_verbose("starting interface");
		}
		return 1;
	}
	else
	{
		output_fatal("info subject not specified");
		/*	TROUBLESHOOT
			The <b>--info</b> command line directive
			was not followed by a valid subject to lookup.
			The correct syntax is <b>--info <i>subject</i></b>.
		 */
		return CMDERR;
	}
}
static int slave(int argc, char *argv[])
{
	char host[256], port[256];

	if ( argc < 2 )
	{
		output_error("slave connection parameters are missing");
		return CMDERR;
	}

	IN_MYCONTEXT output_debug("slave()");
	if(2 != sscanf(argv[1],"%255[^:]:%255s",host,port))
	{
		output_error("unable to parse slave parameters");
	}

	strncpy(global_master,host,sizeof(global_master)-1);
	if ( strcmp(global_master,"localhost")==0 ){
		sscanf(port,"%"FMT_INT64"x",&global_master_port); /* port is actual mmap/shmem */
		global_multirun_connection = MRC_MEM;
	}
	else
	{
		global_master_port = atoi(port);
		global_multirun_connection = MRC_SOCKET;
	}

	if ( FAILED == instance_slave_init() )
	{
		output_error("slave instance init failed for master '%s' connection '%"FMT_INT64"x'", global_master, global_master_port);
		return CMDERR;
	}

	IN_MYCONTEXT output_verbose("slave instance for master '%s' using connection '%"FMT_INT64"x' started ok", global_master, global_master_port);
	return 1;
}

static int slavenode(int argc, char *argv[])
{
	exec_slave_node();
	return CMDOK;
}

static int slave_id(int argc, char *argv[]){
	if(argc < 2){
		output_error("--id requires an ID number argument");
		return CMDERR;
	}
	if(1 != sscanf(argv[1], "%"FMT_INT64"d", &global_slave_id)){
		output_error("slave_id(): unable to read ID number");
		return CMDERR;
	}
	IN_MYCONTEXT output_debug("slave using ID %"FMT_INT64"d", global_slave_id);
	return 1;
}
static int example(int argc, char *argv[])
{
	MODULE *module;
	CLASS *oclass;
	OBJECT *object;
	char modname[1024], classname[1024];
	int n;
	char buffer[65536];
	
	if ( argc < 2 ) 
	{
		output_error("--example requires a module:class argument");
		return CMDERR;
	}
	
	n = sscanf(argv[1],"%1023[A-Za-z_]:%1024[A-Za-z_0-9]",modname,classname);
	if ( n!=2 )
	{
		output_error("--example: %s name is not valid",n==0?"module":"class");
		return CMDERR;
	}
	module = module_load(modname,0,NULL);
	if ( module==NULL )
	{
		output_error("--example: module %d is not found", modname);
		return CMDERR;
	}
	oclass = class_get_class_from_classname(classname);
	if ( oclass==NULL )
	{
		output_error("--example: class %d is not found", classname);
		return CMDERR;
	}
	object = object_create_single(oclass);
	if ( object==NULL )
	{
		output_error("--example: unable to create example object from class %s", classname);
		return CMDERR;
	}
	global_clock = time(NULL);
	output_redirect("error",NULL);
	output_redirect("warning",NULL);
	if ( !object_init(object) )
		output_warning("--example: unable to initialize example object from class %s", classname);
	if ( object_save(buffer,sizeof(buffer),object)>0 )
		output_raw("%s\n", buffer);
	else
		output_warning("no output generated for object");
	return CMDOK;
}
static int mclassdef(int argc, char *argv[])
{
	MODULE *module;
	CLASS *oclass;
	OBJECT *obj;
	char modname[1024], classname[1024];
	int n;
	char buffer[65536];
	PROPERTY *prop;
	int count = 0;

	/* generate the object */
	if ( argc < 2 )
	{
		output_error("--mclassdef requires a module:class argument");
		return CMDERR;
	}
	n = sscanf(argv[1],"%1023[A-Za-z_]:%1024[A-Za-z_0-9]",modname,classname);
        if ( n!=2 )
        {
                output_error("--mclassdef: %s name is not valid",n==0?"module":"class");
                return CMDERR;
        }
        module = module_load(modname,0,NULL);
        if ( module==NULL )
        {
                output_error("--mclassdef: module %d is not found", modname);
                return CMDERR;
        }
        oclass = class_get_class_from_classname(classname);
        if ( oclass==NULL )
        {
                output_error("--mclassdef: class %d is not found", classname);
                return CMDERR;
        }
        obj = object_create_single(oclass);
        if ( obj==NULL )
        {
                output_error("--mclassdef: unable to create mclassdef object from class %s", classname);
                return CMDERR;
        }
        global_clock = time(NULL);
        output_redirect("error",NULL);
        output_redirect("warning",NULL);
        if ( !object_init(obj) )
                output_warning("--mclassdef: unable to initialize mclassdef object from class %s", classname);
	
	/* output the classdef */
	count = sprintf(buffer,"struct('module','%s','class','%s'", modname, classname);
	for ( prop = oclass->pmap ; prop!=NULL && prop->oclass==oclass ; prop=prop->next )
	{
		char temp[1024];
		char *value = object_property_to_string(obj, prop->name, temp, 1023);
		if ( strchr(prop->name,'.')!=NULL )
			continue; /* do not output structures */
		if ( value!=NULL )
                       count += sprintf(buffer+count, ",...\n\t'%s','%s'", prop->name, value);
	}
	count += sprintf(buffer+count,");\n");
	output_raw("%s",buffer);
        return CMDOK;
}
static int locktest(int argc, char *argv[])
{
	test_lock();
	return CMDOK;
}

static int workdir(int argc, char *argv[])
{
	if ( argc<2 )
	{
		output_error("--workdir requires a directory argument");
		return CMDERR;
	}
	strcpy(global_workdir,argv[1]);
	if ( chdir(global_workdir)!=0 )
	{
		output_error("%s is not a valid workdir", global_workdir);
		return CMDERR;
	}
	getcwd(global_workdir,sizeof(global_workdir));
	IN_MYCONTEXT output_verbose("working directory is '%s'", global_workdir);
	return 1;
}

static int local_daemon(int argc, char *argv[])
{
	if ( argc < 2 )
	{
		output_error("--daemon requires a command");
		return CMDERR;
	}
	else if ( strcmp(argv[1],"start") == 0 )
	{
		return daemon_start(argc-1,argv+1);
	}
	else if ( strcmp(argv[1],"stop") == 0 )
	{
		return daemon_stop(argc-1,argv+1);
	}
	else if ( strcmp(argv[1],"restart") == 0 )
	{
		return daemon_restart(argc-1,argv+1);
	}
	else if ( strcmp(argv[1],"status") == 0 )
	{
		return daemon_status(argc-1,argv+1);
	}
	else
	{
		output_error("%s is not a valid daemon command", argv[1]);
		return CMDERR;
	}
}

static int remote_client(int argc, char *argv[])
{
	if ( argc < 2 )
	{
		output_error("--remote requires hostname");
		return CMDERR;
	}
	else
		return daemon_remote_client(argc,argv);
}

static int printenv(int argc, char *argv[])
{
	system("printenv");
	return CMDOK;
}

static int origin(int argc, char *argv[])
{
	FILE *fp;
	char originfile[1024];
	if ( find_file("origin.txt",NULL,R_OK,originfile,sizeof(originfile)-1) == NULL )
	{
		IN_MYCONTEXT output_error("origin file not found");
		return CMDERR;
	}
	fp = fopen(originfile,"r");
	if ( fp == NULL )
	{
		IN_MYCONTEXT output_error("unable to open origin file");
		return CMDERR;
	}
	while ( ! feof(fp) )
	{
		char line[1024];
		size_t len = fread(line,sizeof(line[0]),sizeof(line)-1,fp);
		if ( ferror(fp) )
		{
			IN_MYCONTEXT output_error("error reading origin file");
			return CMDERR;
		}
		if ( len >= 0 )
		{
			int old = global_suppress_repeat_messages;
			global_suppress_repeat_messages = 0;
			line[len] = '\0';
			IN_MYCONTEXT output_message("%s",line);
			global_suppress_repeat_messages = old;
		}
	}
	fclose(fp);
	return 1;
}

#include "job.h"
#include "validate.h"

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
	{"mt_profile",	NULL,	mt_profile,		"<n-threads>", "Analyses multithreaded performance profile" },
	{"profile",		NULL,	profile,		NULL, "Toggles performance profiling of core and modules while simulation runs" },
	{"quiet",		"q",	quiet,			NULL, "Toggles suppression of all but error and fatal messages" },
	{"verbose",		"v",	verbose,		NULL, "Toggles output of verbose messages" },
	{"warn",		"w",	warn,			NULL, "Toggles display of warning messages" },
	{"workdir",		"W",	workdir,		NULL, "Sets the working directory" },
	
	{NULL,NULL,NULL,NULL, "Global, environment and module information"},
	{"define",		"D",	define,			"<name>=[<module>:]<value>", "Defines or sets a global (or module) variable" },
	{"globals",		NULL,	globals,		NULL, "Displays a sorted list of all global variables" },
	{"libinfo",		"L",	libinfo,		"<module>", "Displays information about a module" },
	{"printenv",	"E",	printenv,		NULL, "Displays the default environment variables" },

	{NULL,NULL,NULL,NULL, "Information"},
	{"copyright",	NULL,	copyright,		NULL, "Displays copyright" },
	{"license",		NULL,	license,		NULL, "Displays the license agreement" },
	{"version",		"V",	version,		NULL, "Displays the version information" },
	{"setup",		NULL,	setup,			NULL, "Open simulation setup screen" },
	{"origin",		NULL,	origin,			NULL, "Display origin information" },

	{NULL,NULL,NULL,NULL, "Test processes"},
	{"dsttest",		NULL,	dsttest,		NULL, "Perform daylight savings rule test" },
	{"endusetest",	NULL,	endusetest,		NULL, "Perform enduse pseudo-object test" },
	{"globaldump",	NULL,	globaldump,		NULL, "Perform a dump of the global variables" },
	{"loadshapetest", NULL,	loadshapetest,	NULL, "Perform loadshape pseudo-object test" },
	{"locktest",	NULL,	locktest,		NULL, "Perform memory locking test" },
	{"modtest",		NULL,	modtest,		"<module>", "Perform test function provided by module" },
	{"randtest",	NULL,	randtest,		NULL, "Perform random number generator test" },
	{"scheduletest", NULL,	scheduletest,	NULL, "Perform schedule pseudo-object test" },	
	{"test",		NULL,	test,			"<module>", "Perform unit test of module (deprecated)" },
	{"testall",		NULL,	testall,		"=<filename>", "Perform tests of modules listed in file" },
	{"unitstest",	NULL,	unitstest,		NULL, "Perform unit conversion system test" },
	{"validate",	NULL,	validate,		"...", "Perform model validation check" },

	{NULL,NULL,NULL,NULL, "File and I/O Formatting"},
	{"kml",			NULL,	kml,			"[=<filename>]", "Output to KML (Google Earth) file of model (only supported by some modules)" },
	{"stream",		NULL,	_stream,		NULL, "Toggles streaming I/O" },
	{"sanitize",	NULL,	sanitize,		"<options> <indexfile> <outputfile>", "Output a sanitized version of the GLM model"},
	{"xmlencoding",	NULL,	xmlencoding,	"8|16|32", "Set the XML encoding system" },
	{"xmlstrict",	NULL,	xmlstrict,		NULL, "Toggle strict XML formatting (default is enabled)" },
	{"xsd",			NULL,	xsd,			"[module[:class]]", "Prints the XSD of a module or class" },
	{"xsl",			NULL,	xsl,			"module[,module[,...]]]", "Create the XSL file for the module(s) listed" },

	{NULL,NULL,NULL,NULL, "Help"},
	{"help",		"h",		help,		NULL, "Displays command line help" },
	{"info",		NULL,		info,		"<subject>", "Obtain online help regarding <subject>"},
	{"modhelp",		NULL,		modhelp,	"module[:class]", "Display structure of a class or all classes in a module" },
	{"modlist",		NULL,		modlist,	NULL, "Display list of available modules"},
	{"example",		NULL,		example,	"module:class", "Display an example of an instance of the class after init" },
	{"mclassdef",		NULL,		mclassdef,	"module:class", "Generate Matlab classdef of an instance of the class after init" },

	{NULL,NULL,NULL,NULL, "Process control"},
	{"pidfile",		NULL,	pidfile,		"[=<filename>]", "Set the process ID file (default is gridlabd.pid)" },
	{"threadcount", "T",	threadcount,	"<n>", "Set the maximum number of threads allowed" },
	{"job",			NULL,	job,			"...", "Start a job"},

	{NULL,NULL,NULL,NULL, "System options"},
	{"avlbalance",	NULL,	avlbalance,		NULL, "Toggles automatic balancing of object index" },
	{"bothstdout",	NULL,	bothstdout,		NULL, "Merges all output on stdout" },
	{"check_version", NULL,	_check_version,	NULL, "Perform online version check to see if any updates are available" },
	{"compile",		"C",	compile,		NULL, "Toggles compile-only flags" },
	{"environment",	"e",	environment,	"<appname>", "Set the application to use for run environment" },
	{"output",		"o",	output,			"<file>", "Enables save of output to a file (default is gridlabd.glm)" },
	{"pause",		NULL,	pauseatexit,			NULL, "Toggles pause-at-exit feature" },
	{"relax",		NULL,	relax,			NULL, "Allows implicit variable definition when assignments are made" },

	{NULL,NULL,NULL,NULL, "Server mode"},
	{"server",		NULL,	server,			NULL, "Enables the server"},
	{"daemon",		"d",	local_daemon,	"<command>", "Controls the daemon process"},
	{"remote",		"r",	remote_client,	"<command>", "Connects to a remote daemon process"},
	{"clearmap",	NULL,	clearmap,		NULL, "Clears the process map of defunct jobs (deprecated form)" },
	{"pclear",		NULL,	clearmap,		NULL, "Clears the process map of defunct jobs" },
	{"pcontrol",	NULL,	pcontrol,		NULL, "Enters process controller" },
	{"pkill",		NULL,	pkill,			"<procnum>", "Kills a run on a processor" },
	{"plist",		NULL,	plist,			NULL, "List runs on processes" },
	{"pstatus",		NULL,	pstatus,		NULL, "Prints the process list" },
	{"redirect",	NULL,	redirect,		"<stream>[:<file>]", "Redirects an output to stream to a file (or null)" },
	{"server_portnum", "P", server_portnum, NULL, "Sets the server port number (default is 6267)" },
	{"server_inaddr", NULL, server_inaddr,	NULL, "Sets the server interface address (default is INADDR_ANY, any interface)"},
	{"slave",		NULL,	slave,			"<master>", "Enables slave mode under master"},
	{"slavenode",	NULL,	slavenode,		NULL, "Sets a listener for a remote GridLAB-D call to run in slave mode"},
	{"id",			NULL,	slave_id,		"<idnum>", "Sets the ID number for the slave to inform its using to the master"},
};

int cmdarg_runoption(const char *value)
{
	int i, n;
	char option[64], params[1024]="";
	if ( (n=sscanf(value,"%63s %1023[^\n]", option,params))>0 )
	{
		for ( i=0 ; i<sizeof(main)/sizeof(main[0]) ; i++ )
		{
			if ( main[i].lopt!=NULL && strcmp(main[i].lopt,option)==0 )
				return main[i].call(n,(void*)&params);
		}
	}
	return 0;
}

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

	/* special case for no args */
	if (argc==1)
		return no_cmdargs();

	/* process command arguments */
	while (argv++,--argc>0)
	{
		int found = 0;
		int i;
		for ( i=0 ; i<sizeof(main)/sizeof(main[0]) ; i++ )
		{
			CMDARG arg = main[i];
			char tmp[1024];
			sprintf(tmp,"%s=",arg.lopt);
			if ( ( arg.sopt && strncmp(*argv,"-",1)==0 && strcmp((*argv)+1,arg.sopt)==0 ) 
			  || ( arg.lopt && strncmp(*argv,"--",2)==0 && strcmp((*argv)+2,arg.lopt)==0 ) 
			  || ( arg.lopt && strncmp(*argv,"--",2)==0 && strncmp((*argv)+2,tmp,strlen(tmp))==0 ) )
			{
				int n = arg.call(argc,argv);
				switch (n) {
				case CMDOK:
					return status;
				case CMDERR:
					return FAILED;
				default:
					found = 1;
					argc -= n;
					argv += n;
				}
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

					/* preserve name of first model only */
					if (strcmp(global_modelname,"")==0)
						strcpy(global_modelname,*argv);

					if (!loadall(*argv))
						status = FAILED;
					loader_time += clock() - start;
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
