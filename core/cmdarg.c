/** $Id: cmdarg.c 1182 2008-12-22 22:08:36Z dchassin $
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
	int test_mod_num = 1;
	unsigned int pos=0;
	int i;
	char *pd1, *pd2;

	/* capture the execdir */
	strcpy(global_execname,argv[0]);
	strcpy(global_execdir,argv[0]);
	pd1 = strrchr(global_execdir,'/');
	pd2 = strrchr(global_execdir,'\\');
	if (pd1>pd2) *pd1='\0';
	else if (pd2>pd1) *pd2='\0';

	/* capture the command line */
	for (i=0; i<argc; i++)
	{
		if (pos<sizeof(global_command_line)-strlen(argv[i]))
			pos += sprintf(global_command_line+pos,"%s%s",pos>0?" ":"",argv[i]);
	}

	while (argv++,--argc>0)
	{
		if (strcmp(*argv,"--copyright")==0)
			legal_notice();
		else if (strcmp(*argv,"-w")==0 || strcmp(*argv,"--warn")==0)
			global_warn_mode=!global_warn_mode;
		else if (strcmp(*argv,"--bothstdout")==0)
			output_both_stdout();
		else if (strcmp(*argv,"-c")==0 || strcmp(*argv,"--check")==0)
			global_runchecks=!global_runchecks;
		else if (strcmp(*argv,"--debug")==0)
			global_debug_output=!global_debug_output;
		else if (strcmp(*argv,"--debugger")==0){
			global_debug_mode=1;
			global_debug_output=!global_debug_output;
		}
		else if (strcmp(*argv,"--dumpall")==0)
			global_dumpall=!global_dumpall;
		else if (strcmp(*argv,"-q")==0 || strcmp(*argv,"--quiet")==0)
			global_quiet_mode=!global_quiet_mode;
		else if (strcmp(*argv,"-v")==0 || strcmp(*argv,"--verbose")==0)
			global_verbose_mode=!global_verbose_mode;
		else if (strcmp(*argv,"--check_version")==0)
			check_version(0);
		else if (strcmp(*argv,"--profile")==0)
			global_profiler=!global_profiler;
		else if (strcmp(*argv,"--pause")==0)
			global_pauseatexit=!global_pauseatexit;
		else if (strcmp(*argv,"--compile")==0)
			global_compileonly = !global_compileonly;
		else if (strcmp(*argv,"--license")==0)
			legal_license();
		else if (strcmp(*argv,"--server_portnum")==0 || strcmp(*argv,"-P")==0)
		{
			if (argc-1>0)
				global_server_portnum = (argc--,atoi(*++argv));
			else
			{
				output_fatal("missing server port number");
				/*	TROUBLESHOOT
					The <b>-P</b> or <b>--server_portnum</b> command line directive
					was not followed by a valid number.  The correct syntax is
					<b>-P <i>number</i></b> or <b>--server_portnum <i>number</i></b>.
				 */
			}
		}
		else if (strcmp(*argv, "-V")==0 ||strcmp(*argv, "--version")==0)
		{
#ifdef VERSION
			output_message("%s %s",PACKAGE_STRING, BRANCH);
#else
			output_message("GridLAB-D %d.%d.%d.%s %s",REV_MAJOR,REV_MINOR,REV_PATCH,BUILD,BRANCH);
#endif
		}
		else if (strcmp(*argv,"--dsttest")==0)
			timestamp_test();
		else if (strcmp(*argv,"--randtest")==0)
			random_test();
		else if (strcmp(*argv,"--unitstest")==0)
			unit_test();
		else if (strcmp(*argv,"--scheduletest")==0)
			schedule_test();
		else if (strcmp(*argv,"--loadshapetest")==0)
			loadshape_test();
		else if (strcmp(*argv,"--endusetest")==0)
			enduse_test();
		else if (strcmp(*argv,"--xmlstrict")==0)
			global_xmlstrict = !global_xmlstrict;
		else if (strcmp(*argv,"--globaldump")==0)
		{
			global_dump();
			exit(0);
		}
		else if (strcmp(*argv,"--relax")==0)
			global_strictnames = FALSE;
		else if (strncmp(*argv,"--pidfile",9)==0)
		{
			char *filename = strchr(*argv,'=');
			if (filename==NULL)
				strcpy(global_pidfile,"gridlabd.pid");
			else
				strcpy(global_pidfile,filename+1);
		}
		else if (strncmp(*argv,"--kml",5)==0)
		{
			char *filename = strchr(*argv,'=');
			if (filename)
				strcpy(global_kmlfile,filename+1);
			else
				strcpy(global_kmlfile,"gridlabd.kml");
		}
		else if (strcmp(*argv, "--avlbalance") == 0){
			global_no_balance = !global_no_balance;
		}
		else if (strcmp(*argv,"--testall")==0){
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
				return FAILED;
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
				return FAILED;
			}
			if(load_module_list(fd,&test_mod_num) == FAILED)
				return FAILED;
		}
		else if (strcmp(*argv,"--modhelp")==0)
		{
			if(argc-1 > 0){
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
		}
		else if (strcmp(*argv,"--modtest")==0)
		{
			if (argc-1>0)
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
		}
		else if (strcmp(*argv,"--test")==0){
			global_test_mode=TRUE;
			global_strictnames = FALSE;
			output_debug("disabling strict naming for tests");
			if (argc-1>0)
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
				return FAILED;
			}

		}
		else if (strcmp(*argv,"-D")==0 || strcmp(*argv,"--define")==0)
		{
			if (argc-1>0)
			{
				bool namestate = global_strictnames;
				global_strictnames = FALSE;
				if (global_setvar(*++argv,NULL)==SUCCESS){
					argc--;
				}
				global_strictnames = namestate;
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
				return FAILED;
			}
		}
		else if (strcmp(*argv,"--globals")==0)
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
		}
		else if (strcmp(*argv,"--redirect")==0)
		{
			if (argc-1>0)
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
		}
		else if (strcmp(*argv,"-L")==0 || strcmp(*argv,"--libinfo")==0)
		{
			if (argc-1>0)
			{	argc--;
				module_libinfo(*++argv);
				exit(0);
			}
			else
			{
				output_fatal("missing library name");
				/*	TROUBLESHOOT
					The <b>-L</b> or <b>--libinfo</b> command line directive
					was not followed by a module name.  The correct syntax is
					<b>-L <i>module_name</i></b> or <b>--libinfo <i>module_name</i></b>.
				 */
				return FAILED;
			}
		}
		else if (strcmp(*argv,"-T")==0 || strcmp(*argv,"--threadcount")==0)
		{
			if (argc-1>0)
				global_threadcount = (argc--,atoi(*++argv));
			else
			{
				output_fatal("missing thread count");
				/*	TROUBLESHOOT
					The <b>-T</b> or <b>--threadcount</b> command line directive
					was not followed by a valid number.  The correct syntax is
					<b>-T <i>number</i></b> or <b>--threadcount <i>number</i></b>.
				 */
				return FAILED;
			}
		}
		else if (strcmp(*argv,"-o")==0 || strcmp(*argv,"--output")==0)
		{
			if (argc-1>0)
				strcpy(global_savefile,(argc--,*++argv));
			else
			{
				output_fatal("missing output file");
				/* TROUBLESHOOT
					The <b>-o</b> or <b>--output</b> command line directive
					was not followed by a valid filename.  The correct syntax is
					<b>-o <i>file</i></b> or <b>--output <i>file</i></b>.
				 */
				return FAILED;
			}
		}
		else if (strcmp(*argv,"-e")==0 || strcmp(*argv,"--environment")==0)
		{
			if (argc-1>0)
				strcpy(global_environment,(argc--,*++argv));
			else
			{
				output_fatal("environment not specified");
				/*	TROUBLESHOOT
					The <b>-e</b> or <b>--environment</b> command line directive
					was not followed by a valid environment specification.  The
					correct syntax is <b>-e <i>keyword</i></b> or <b>--environment <i>keyword</i></b>.
				 */
				return FAILED;
			}
		}
		else if (strcmp(*argv,"--xmlencoding")==0)
		{
			if (argc-1>0)
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
		}
		else if (strcmp(*argv,"--xsd")==0)
		{
			if (argc-1>0)
			{
				argc--;
				exit(output_xsd(*++argv));
			}
			else
			{
				MODULE *mod;
				for (mod=module_get_first(); mod!=NULL; mod=mod->next)
					output_xsd(mod->name);
				return SUCCESS;
			}
		}
		else if (strcmp(*argv,"--xsl")==0)
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
				exit(output_xsl(fname,n_args,p_args));
			}
			else
			{
				output_fatal("module list not specified");
				/*	TROUBLESHOOT
					The <b>--xsl</b> command line directive
					was not followed by a validlist of modules.  The
					correct syntax is <b>--xsl <i>module1</i>[,<i>module2</i>[,...]]</b>.
				 */
				return FAILED;
			}
		}
		else if (strcmp(*argv,"--stream")==0)
			global_streaming_io_enabled = !global_streaming_io_enabled;
		else if (strcmp(*argv,"--server")==0)
			strcpy(global_environment,"server");
		else if (strcmp(*argv,"-h")==0 || strcmp(*argv,"--help")==0)
		{
			printf("Syntax: gridlabd [OPTIONS ...] <file> ... \nOptions:\n"
				"  --avlbalance              toggles AVL tree balancing\n"
				"  -c|--check                toggles module checks after model loads\n"
				"  -D|--define <def>         defines a macro value\n"
				"  --debug                   toggles debug output (prints internal messages)\n"
				"  --debugger                toggles debugger mode (generates internal messages)\n"
				"  --dumpall                 toggles module data dump after run completes\n"
				"  -e|--environment <name>   specifies user environment (default none)\n"
				"  --license                 print license information\n"
				"  -L|--libinfo <module>     print module library information\n"
				"  -o|--output <file>        specifies model should be output after run\n"
				"  --profile                 toggles profilers\n"
				"  -q|--quiet                toggles quiet mode (suppresses startup banner)\n"
				"  --test                    toggles test mode (activate testing procedures)\n"
				"  -T|--threadcount <n>      specifies the number of processor threads to use\n"
				"  -v|--verbose              toggles verbose mode (active verbose messages)\n"
				"  -V|--version              prints the GridlabD version information\n"
				"  -w|--warn                 toggles warning mode (generates warning messages)\n"
				"  --xmlencoding <num>       set the XML encoding (8, 16, or 32)\n"
				"  --xmlstrict               toggles XML encoding to be strict\n"
				"  --xsd <module>[:<object>] prints the xsd of an object\n"
				"  --xsl <modlist>           prints the xsl for the modules listed\n"
				);
			exit(0);
		}
		else if (**argv!='-')
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
					return FAILED;
				loader_time += clock() - start;

				/* preserve name of first model only */
				if (strcmp(global_modelname,"")==0)
					strcpy(global_modelname,*argv);
			}
		}
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
				return FAILED;
			}
		}
	}
	/*debug_traverse_tree(NULL);*/  /* for checking the name tree & getting a test file. -mh */
	return SUCCESS;
}

/**@}**/
