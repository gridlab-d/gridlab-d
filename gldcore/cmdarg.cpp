/** $Id: cmdarg.c 5518 2016-07-15 00:55:28Z andyfisher $
	Copyright (C) 2008 Battelle Memorial Institute
	@file cmdarg.c
	@addtogroup cmdarg Command-line arguments
	@ingroup core

	The command-line argument processing module processes arguments as they are encountered.
	
	Use the --help command line option to obtain a list of valid options.

 @{
 **/

#include <json/json.h> 
#include <cstdio>
#include <cstring>
#if defined(_WIN32) && !defined(__MINGW__)
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
#include "gldrandom.h"
#include "loadshape.h"
#include "enduse.h"
#include "instance.h"
#include "test.h"
#include "setup.h"
#include "sanitize.h"
#include "exec.h"
#include "module.h"

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
		//printf("ERROR %s", oclass->name); /* exception? */
		return;
	} 
	if(cmpval < 0){ /*  class < root ~ go left */
		if(targ->left == nullptr){
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
		if(targ->right == nullptr){
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

void jprint_class_d(CLASS *oclass, int tabdepth, Json::Value& _module){
	PROPERTY *prop;
	Json::Value _class;

	if (oclass->parent) {
		_class[oclass->parent->name]["type"] = "parent";
	}
	for (prop=oclass->pmap; prop!=nullptr && prop->oclass==oclass; prop=prop->next) {
		const char *propname = class_get_property_typename(prop->ptype);
		if (propname!=nullptr) {
			if ( (prop->access&PA_HIDDEN)==PA_HIDDEN )
				continue;

			Json::Value _property;
			if (prop->unit != nullptr) {
				_property["type"] = propname;
				_property["unit"] = prop->unit->name;
			}
			else if (prop->ptype==PT_set || prop->ptype==PT_enumeration) {
				KEYWORD *key;
				_property["type"] = propname;
				for (key=prop->keywords; key!=nullptr; key=key->next)
					_property["keywords"][key->name] = key->value;
			} 
			else {
				_property["type"] = propname;
			}
			if (prop->description!=nullptr)
				_property["description"] = prop->description;
			if (prop->flags&PF_DEPRECATED)
				_property["deprecated"] = true;

			_class[prop->name] = _property;
		}
	}
	_module[oclass->name] = _class;
}
void jprint_class(CLASS *oclass, Json::Value& _module){
	jprint_class_d(oclass, 0, _module);
}
void jprint_modhelp_tree(pntree *ctree, Json::Value& _module){
	if(ctree->left != nullptr){
		jprint_modhelp_tree(ctree->left, _module);
		free(ctree->left);
		ctree->left = 0;
	}
	jprint_class(ctree->oclass, _module);
	if(ctree->right != nullptr){
		jprint_modhelp_tree(ctree->right, _module);
		free(ctree->right);
		ctree->right = 0;
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
	for (func=oclass->fmap; func!=nullptr && func->oclass==oclass; func=func->next)
		printf( "%s\tfunction %s();\n", tabs, func->name);
	for (prop=oclass->pmap; prop!=nullptr && prop->oclass==oclass; prop=prop->next)
	{
		const char *propname = class_get_property_typename(prop->ptype);
		if (propname!=nullptr)
		{
			if ( (prop->access&PA_HIDDEN)==PA_HIDDEN )
				continue;
			if (prop->unit != nullptr)
			{
				printf("%s\t%s %s[%s];", tabs, propname, prop->name, prop->unit->name);
			}
			else if (prop->ptype==PT_set || prop->ptype==PT_enumeration)
			{
				KEYWORD *key;
				printf("%s\t%s {", tabs, propname);
				for (key=prop->keywords; key!=nullptr; key=key->next)
					printf("%s=%" FMT_INT64 "u%s", key->name, (int64)key->value, key->next==nullptr?"":", ");
				printf("} %s;", prop->name);
			} 
			else 
			{
				printf("%s\t%s %s;", tabs, propname, prop->name);
			}
			if (prop->description!=nullptr)
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
	if(ctree->left != nullptr){
		print_modhelp_tree(ctree->left);
		free(ctree->left);
		ctree->left = 0;
	}
	print_class(ctree->oclass);
	if(ctree->right != nullptr){
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
	const char *lopt;
	const char *sopt;
	int (*call)(int argc, char *argv[]);
	const char *args;
	const char *desc;
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

#define CMDOK (-1)
#define CMDERR (-2)

static STATUS no_cmdargs()
{
	char htmlfile[1024];
	if ( global_autostartgui && find_file("gridlabd.htm",nullptr,R_OK,htmlfile,sizeof(htmlfile)-1)!=nullptr )
	{
		char cmd[1024];

		/* enter server mode and wait */
#ifdef _WIN32
		if ( htmlfile[1]!=':' )
			sprintf(htmlfile,"%s\\gridlabd.htm", global_workdir);
		output_message("opening html page '%s'", htmlfile);
		sprintf(cmd,"start %s file:///%s", global_browser, htmlfile);
#elif defined(MACOSX)
		sprintf(cmd,"open -a %s %s", global_browser, htmlfile);
#else
		sprintf(cmd,"%s '%s' & ps -p $! >/dev/null", global_browser, htmlfile);
#endif
		output_verbose("Starting browser using command [%s]", cmd);
		if (system(cmd)!=0)
		{
			output_error("unable to start browser");
			return FAILED;
		}
		else
			output_verbose("starting interface");
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
	output_message("GridLAB-D %d.%d.%d-%d (%.48s) %d-bit %s %s", 
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
	output_verbose("xmlstrict is %s", global_xmlstrict?"enabled":"disabled");
	return 0;
}
static int globaldump(int argc, char *argv[])
{
	global_dump();
	return CMDOK;
}
static int relax(int argc, char *argv[])
{
	global_strictnames = false;
	return 0;
}
static int pidfile(int argc, char *argv[])
{
	char *filename = strchr(*argv,'=');
	if (filename==nullptr)
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
	FILE *fd = nullptr;
	if(*++argv != nullptr)
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
	global_test_mode=true;

	if(fd == nullptr)
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
static int modattr(int argc, char *argv[])
{
	if(argc > 1){
		MODULE *mod = nullptr;
		CLASS *oclass = nullptr;
		argv++;
		argc--;
		if(strchr(argv[0], ':') == 0){ // no class
			mod = module_load(argv[0],0,nullptr);
		} else {
			mod = module_load(strtok(argv[0],":"),0,nullptr);
		}

		Json::Value _module;
		Json::Value _global;
		GLOBALVAR *var=nullptr;
			/* dump module globals */
		while ((var=global_getnext(var))!=nullptr)
		{
			PROPERTY *prop = var->prop;
			const char *proptype = class_get_property_typename(prop->ptype);
			if ( strncmp(var->prop->name,mod->name,strlen(mod->name))!=0 )
				continue;
			if ( (prop->access&PA_HIDDEN)==PA_HIDDEN )
				continue;
			if (proptype!=nullptr)
			{
				Json::Value _property;
				if ( prop->unit!=nullptr )
				{
					_property["type"] = proptype;
					_property["unit"] = prop->unit->name;
				}
				else if (prop->ptype==PT_set || prop->ptype==PT_enumeration)
				{
					KEYWORD *key;
					_property["type"] = proptype;
					for (key=prop->keywords; key!=nullptr; key=key->next)
						_property["keywords"][key->name] = key->value;
				} 
				else 
				{
					_property["type"] = proptype;
				}
				if (prop->description!=nullptr)
					_property["description"] = prop->description;
				if (prop->flags&PF_DEPRECATED)
					_property["deprecated"] = true;

				_global[prop->name] = _property;
			}
		}
		_module["global_attributes"] = _global;

		if(mod == nullptr){
			output_fatal("module %s is not found",*argv);
			/*	TROUBLESHOOT
				The <b>--modhelp</b> parameter was found on the command line, but
				if was followed by a module specification that isn't valid.
				Verify that the module exists in GridLAB-D's <b>lib</b> folder.
			*/
			return FAILED;
		}
		pntree	*ctree;
		/* lexographically sort all elements from class_get_first_class & oclass->next */

		oclass=class_get_first_class();
		ctree = (pntree *)malloc(sizeof(pntree));
		
		if(ctree == nullptr){
			throw_exception("--modhelp: malloc failure");
			/* TROUBLESHOOT
				The memory allocation needed for module help to function has failed.  Try freeing up system memory and try again.
			 */
		}
		
		ctree->name = oclass->name;
		ctree->oclass = oclass;
		ctree->left = ctree->right = 0;
		
		for(; oclass != nullptr; oclass = oclass->next){
			modhelp_alpha(&ctree, oclass);
		}

		/* flatten tree */
		jprint_modhelp_tree(ctree, _module);
		Json::StreamWriterBuilder builder;
		builder["indentation"] = "  ";
		std::cout << Json::writeString(builder, _module);
	}
	return 1;
}
static int modhelp(int argc, char *argv[])
{
	if(argc > 1){
		MODULE *mod = nullptr;
		CLASS *oclass = nullptr;
		argv++;
		argc--;
		if(strchr(argv[0], ':') == 0){ // no class
			mod = module_load(argv[0],0,nullptr);
		} else {
			GLOBALVAR *var=nullptr;
			char *cname;
			cname = strchr(argv[0], ':')+1;
			mod = module_load(strtok(argv[0],":"),0,nullptr);
			oclass = class_get_class_from_classname(cname);
			if(oclass == nullptr){
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
			while ((var=global_getnext(var))!=nullptr)
			{
				PROPERTY *prop = var->prop;
				const char *proptype = class_get_property_typename(prop->ptype);
				if ( strncmp(var->prop->name,mod->name,strlen(mod->name))!=0 )
					continue;
				if ( (prop->access&PA_HIDDEN)==PA_HIDDEN )
					continue;
				if (proptype!=nullptr)
				{
					if ( prop->unit!=nullptr )
					{
						printf("\t%s %s[%s];", proptype, strrchr(prop->name,':')+1, prop->unit->name);
					}
					else if (prop->ptype==PT_set || prop->ptype==PT_enumeration)
					{
						KEYWORD *key;
						printf("\t%s {", proptype);
						for (key=prop->keywords; key!=nullptr; key=key->next)
							printf("%s=%ld%s", key->name, key->value, key->next==nullptr?"\n":",\n");
						printf("} %s;", strrchr(prop->name,':')+1);
					} 
					else 
					{
						printf("\t%s %s;", proptype, strrchr(prop->name,':')+1);
					}
					if (prop->description!=nullptr)
						printf(" // %s%s",prop->flags&PF_DEPRECATED?"(DEPRECATED) ":"",prop->description);
					printf("\n");
				}
			}
			printf("}\n");
		}
		if(mod == nullptr){
			output_fatal("module %s is not found",*argv);
			/*	TROUBLESHOOT
				The <b>--modhelp</b> parameter was found on the command line, but
				if was followed by a module specification that isn't valid.
				Verify that the module exists in GridLAB-D's <b>lib</b> folder.
			*/
			return FAILED;
		}
		if(oclass != nullptr)
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
			
			if(ctree == nullptr){
				throw_exception("--modhelp: malloc failure");
				/* TROUBLESHOOT
					The memory allocation needed for module help to function has failed.  Try freeing up system memory and try again.
				 */
			}
			
			ctree->name = oclass->name;
			ctree->oclass = oclass;
			ctree->left = ctree->right = 0;
			
			for(; oclass != nullptr; oclass = oclass->next){
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
		MODULE *mod = module_load(argv[1],0,nullptr);
		if (mod==nullptr)
			output_fatal("module %s is not found",argv[1]);
			/*	TROUBLESHOOT
				The <b>--modtest</b> parameter was found on the command line, but
				if was followed by a module specification that isn't valid.
				Verify that the module exists in GridLAB-D's <b>lib</b> folder.
			*/
		else 
		{
			argv++;argc--;
			if (mod->test==nullptr)
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
				mod->test(0,nullptr);
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
	global_test_mode = true;
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
		global_strictnames = false;
		if (global_setvar(*++argv,nullptr)==SUCCESS){
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
	GLOBALVAR *var = nullptr;

	/* load the list into the array */
	while ((var=global_getnext(var))!=nullptr)
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
			if (output_redirect("output",nullptr)==nullptr ||
				output_redirect("error",nullptr)==nullptr ||
				output_redirect("warning",nullptr)==nullptr ||
				output_redirect("debug",nullptr)==nullptr ||
				output_redirect("verbose",nullptr)==nullptr ||
				output_redirect("profile",nullptr)==nullptr ||
				output_redirect("progress",nullptr)==nullptr)
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
		else if ((p=strchr(buffer,':'))!=nullptr)
		{
			*p++='\0';
			if (output_redirect(buffer,p)==nullptr)
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
		else if (output_redirect(buffer,nullptr)==nullptr)
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
		for (mod=module_get_first(); mod!=nullptr; mod=mod->next)
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
		while (p_arg!=nullptr)
		{
			p_args[n_args++] = p_arg;
			p_arg = strtok(nullptr,",");
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
#ifdef _WIN32
		sprintf(cmd,"start %s \"%s%s\"", global_browser, global_infourl, argv[1]);
#elif defined(MACOSX)
		sprintf(cmd,"open -a %s \"%s%s\"", global_browser, global_infourl, argv[1]);
#else
		sprintf(cmd,"%s \"%s%s\" & ps -p $! >/dev/null", global_browser, global_infourl, argv[1]);
#endif
		output_verbose("Starting browser using command [%s]", cmd);
		if (system(cmd)!=0)
		{
			output_error("unable to start browser");
			return CMDERR;
		}
		else
			output_verbose("starting interface");
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

	output_debug("slave()");
	if(2 != sscanf(argv[1],"%255[^:]:%255s",host,port))
	{
		output_error("unable to parse slave parameters");
	}

	strncpy(global_master,host,sizeof(global_master)-1);
	if ( strcmp(global_master,"localhost")==0 ){
		sscanf(port,"%" FMT_INT64 "x",&global_master_port); /* port is actual mmap/shmem */
		global_multirun_connection = MRC_MEM;
	}
	else
	{
		global_master_port = atoi(port);
		global_multirun_connection = MRC_SOCKET;
	}

	if ( FAILED == instance_slave_init() )
	{
		output_error("slave instance init failed for master '%s' connection '%" FMT_INT64 "x'", global_master, global_master_port);
		return CMDERR;
	}

	output_verbose("slave instance for master '%s' using connection '%" FMT_INT64 "x' started ok", global_master, global_master_port);
	return 1;
}
static int slavenode(int argc, char *argv[])
{
	exec_slave_node();
	return CMDOK;
}
static int slave_id(int argc, char *argv[])
{
	if(argc < 2){
		output_error("--id requires an ID number argument");
		return CMDERR;
	}
	if(1 != sscanf(argv[1], "%" FMT_INT64 "d", &global_slave_id)){
		output_error("slave_id(): unable to read ID number");
		return CMDERR;
	}
	output_debug("slave using ID %" FMT_INT64 "d", global_slave_id);
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
	module = module_load(modname,0,nullptr);
	if ( module==nullptr )
	{
		output_error("--example: module %d is not found", modname);
		return CMDERR;
	}
	oclass = class_get_class_from_classname(classname);
	if ( oclass==nullptr )
	{
		output_error("--example: class %d is not found", classname);
		return CMDERR;
	}
	object = object_create_single(oclass);
	if ( object==nullptr )
	{
		output_error("--example: unable to create example object from class %s", classname);
		return CMDERR;
	}
	global_clock = time(nullptr);
	output_redirect("error",nullptr);
	output_redirect("warning",nullptr);
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
        module = module_load(modname,0,nullptr);
        if ( module==nullptr )
        {
                output_error("--mclassdef: module %d is not found", modname);
                return CMDERR;
        }
        oclass = class_get_class_from_classname(classname);
        if ( oclass==nullptr )
        {
                output_error("--mclassdef: class %d is not found", classname);
                return CMDERR;
        }
        obj = object_create_single(oclass);
        if ( obj==nullptr )
        {
                output_error("--mclassdef: unable to create mclassdef object from class %s", classname);
                return CMDERR;
        }
        global_clock = time(nullptr);
        output_redirect("error",nullptr);
        output_redirect("warning",nullptr);
        if ( !object_init(obj) )
                output_warning("--mclassdef: unable to initialize mclassdef object from class %s", classname);
	
	/* output the classdef */
	count = sprintf(buffer,"struct('module','%s','class','%s'", modname, classname);
	for ( prop = oclass->pmap ; prop!=nullptr && prop->oclass==oclass ; prop=prop->next )
	{
		char temp[1024];
		char *value = object_property_to_string(obj, prop->name, temp, 1023);
		if ( strchr(prop->name,'.')!=nullptr )
			continue; /* do not output structures */
		if ( value!=nullptr )
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
static int lock(int argc, char *argv[])
{
	global_lock_enabled = !global_lock_enabled;
	return 0;
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
	output_verbose("working directory is '%s'", getcwd(global_workdir,sizeof(global_workdir)));
	return 1;
}

#include "job.h"
#include "validate.h"

/*********************************************/
/* ADD NEW CMDARG PROCESSORS ABOVE THIS HERE */
/* Then make the appropriate entry in the    */
/* CMDARG structure below                    */
/*********************************************/

static CMDARG main_cmd[] = {

	/* nullptr,nullptr,nullptr,nullptr, "Section heading */
	{nullptr,nullptr,nullptr,nullptr, "Command-line options"},

	/* long_str, short_str, processor_call, arglist_desc, help_desc */
	{"check",		"c",	check,			nullptr, "Performs module checks before starting simulation" },
	{"debug",		nullptr,	debug,			nullptr, "Toggles display of debug messages" },
	{"debugger",	nullptr,	debugger,		nullptr, "Enables the debugger" },
	{"dumpall",		nullptr,	dumpall,		nullptr, "Dumps the global variable list" },
	{"mt_profile",	nullptr,	mt_profile,		"<n-threads>", "Analyses multithreaded performance profile" },
	{"profile",		nullptr,	profile,		nullptr, "Toggles performance profiling of core and modules while simulation runs" },
	{"quiet",		"q",	quiet,			nullptr, "Toggles suppression of all but error and fatal messages" },
	{"verbose",		"v",	verbose,		nullptr, "Toggles output of verbose messages" },
	{"warn",		"w",	warn,			nullptr, "Toggles display of warning messages" },
	{"workdir",		"W",	workdir,		nullptr, "Sets the working directory" },
	{"lock",		"l",	lock,		nullptr, "Toggles read and write locks"},
	
	{nullptr,nullptr,nullptr,nullptr, "Global and module control"},
	{"define",		"D",	define,			"<name>=[<module>:]<value>", "Defines or sets a global (or module) variable" },
	{"globals",		nullptr,	globals,		nullptr, "Displays a sorted list of all global variables" },
	{"libinfo",		"L",	libinfo,		"<module>", "Displays information about a module" },

	{nullptr,nullptr,nullptr,nullptr, "Information"},
	{"copyright",	nullptr,	copyright,		nullptr, "Displays copyright" },
	{"license",		nullptr,	license,		nullptr, "Displays the license agreement" },
	{"version",		"V",	version,		nullptr, "Displays the version information" },
	{"setup",		nullptr,	setup,			nullptr, "Open simulation setup screen" },

	{nullptr,nullptr,nullptr,nullptr, "Test processes"},
	{"dsttest",		nullptr,	dsttest,		nullptr, "Perform daylight savings rule test" },
	{"endusetest",	nullptr,	endusetest,		nullptr, "Perform enduse pseudo-object test" },
	{"globaldump",	nullptr,	globaldump,		nullptr, "Perform a dump of the global variables" },
	{"loadshapetest", nullptr,	loadshapetest,	nullptr, "Perform loadshape pseudo-object test" },
	{"locktest",	nullptr,	locktest,		nullptr, "Perform memory locking test" },
	{"modtest",		nullptr,	modtest,		"<module>", "Perform test function provided by module" },
	{"randtest",	nullptr,	randtest,		nullptr, "Perform random number generator test" },
	{"scheduletest", nullptr,	scheduletest,	nullptr, "Perform schedule pseudo-object test" },
	{"test",		nullptr,	test,			"<module>", "Perform unit test of module (deprecated)" },
	{"testall",		nullptr,	testall,		"=<filename>", "Perform tests of modules listed in file" },
	{"unitstest",	nullptr,	unitstest,		nullptr, "Perform unit conversion system test" },
	{"validate",	nullptr,	validate,		"...", "Perform model validation check" },

	{nullptr,nullptr,nullptr,nullptr, "File and I/O Formatting"},
	{"kml",			nullptr,	kml,			"[=<filename>]", "Output to KML (Google Earth) file of model (only supported by some modules)" },
	{"stream",		nullptr,	_stream,		nullptr, "Toggles streaming I/O" },
	{"sanitize",	nullptr,	sanitize,		"<options> <indexfile> <outputfile>", "Output a sanitized version of the GLM model"},
	{"xmlencoding",	nullptr,	xmlencoding,	"8|16|32", "Set the XML encoding system" },
	{"xmlstrict",	nullptr,	xmlstrict,		nullptr, "Toggle strict XML formatting (default is enabled)" },
	{"xsd",			nullptr,	xsd,			"[module[:class]]", "Prints the XSD of a module or class" },
	{"xsl",			nullptr,	xsl,			"module[,module[,...]]]", "Create the XSL file for the module(s) listed" },

	{nullptr,nullptr,nullptr,nullptr, "Help"},
	{"help",		"h",		help,		nullptr, "Displays command line help" },
	{"info",		nullptr,		info,		"<subject>", "Obtain online help regarding <subject>"},
	{"modattr",		nullptr,		modattr,	"module", "Display structure of all classes in a module and its attributes in json format" },
	{"modhelp",		nullptr,		modhelp,	"module[:class]", "Display structure of a class or all classes in a module" },
	{"modlist",		nullptr,		modlist,	nullptr, "Display list of available modules"},
	{"example",		nullptr,		example,	"module:class", "Display an example of an instance of the class after init" },
	{"mclassdef",		nullptr,		mclassdef,	"module:class", "Generate Matlab classdef of an instance of the class after init" },

	{nullptr,nullptr,nullptr,nullptr, "Process control"},
	{"pidfile",		nullptr,	pidfile,		"[=<filename>]", "Set the process ID file (default is gridlabd.pid)" },
	{"threadcount", "T",	threadcount,	"<n>", "Set the maximum number of threads allowed" },
	{"job",			nullptr,	job,			"...", "Start a job"},

	{nullptr,nullptr,nullptr,nullptr, "System options"},
	{"avlbalance",	nullptr,	avlbalance,		nullptr, "Toggles automatic balancing of object index" },
	{"bothstdout",	nullptr,	bothstdout,		nullptr, "Merges all output on stdout" },
	{"check_version", nullptr,	_check_version,	nullptr, "Perform online version check to see if any updates are available" },
	{"compile",		"C",	compile,		nullptr, "Toggles compile-only flags" },
	{"environment",	"e",	environment,	"<appname>", "Set the application to use for run environment" },
	{"output",		"o",	output,			"<file>", "Enables save of output to a file (default is gridlabd.glm)" },
	{"pause",		nullptr,	pauseatexit,			nullptr, "Toggles pause-at-exit feature" },
	{"relax",		nullptr,	relax,			nullptr, "Allows implicit variable definition when assignments are made" },

	{nullptr,nullptr,nullptr,nullptr, "Server mode"},
	{"server",		nullptr,	server,			nullptr, "Enables the server"},
	{"clearmap",	nullptr,	clearmap,		nullptr, "Clears the process map of defunct jobs (deprecated form)" },
	{"pclear",		nullptr,	clearmap,		nullptr, "Clears the process map of defunct jobs" },
	{"pcontrol",	nullptr,	pcontrol,		nullptr, "Enters process controller" },
	{"pkill",		nullptr,	pkill,			"<procnum>", "Kills a run on a processor" },
	{"plist",		nullptr,	plist,			nullptr, "List runs on processes" },
	{"pstatus",		nullptr,	pstatus,		nullptr, "Prints the process list" },
	{"redirect",	nullptr,	redirect,		"<stream>[:<file>]", "Redirects an output to stream to a file (or null)" },
	{"server_portnum", "P", server_portnum, nullptr, "Sets the server port number (default is 6267)" },
	{"server_inaddr", nullptr, server_inaddr,	nullptr, "Sets the server interface address (default is INADDR_ANY, any interface)"},
	{"slave",		nullptr,	slave,			"<master>", "Enables slave mode under master"},
	{"slavenode",	nullptr,	slavenode,		nullptr, "Sets a listener for a remote GridLAB-D call to run in slave mode"},
	{"id",			nullptr,	slave_id,		"<idnum>", "Sets the ID number for the slave to inform its using to the master"},
};

int cmdarg_runoption(const char *value)
{
	int i, n;
	char option[64], params[1024]="";
	if ( (n=sscanf(value,"%63s %1023[^\n]", option,params))>0 )
	{
		for ( i=0 ; i<sizeof(main_cmd)/sizeof(main_cmd[0]) ; i++ )
		{
			if ( main_cmd[i].lopt!=nullptr && strcmp(main_cmd[i].lopt,option)==0 )
				return main_cmd[i].call(n,(char**)&params);
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

	for ( i=0 ; i<sizeof(main_cmd)/sizeof(main_cmd[0]) ; i++ )
	{
		CMDARG arg = main_cmd[i];
		size_t len = (arg.sopt?strlen(arg.sopt):0) + (arg.lopt?strlen(arg.lopt):0) + (arg.args?strlen(arg.args):0);
		if (len>indent) indent = len;
	}

	for ( i=0 ; i<sizeof(main_cmd)/sizeof(main_cmd[0]) ; i++ )
	{
		CMDARG arg = main_cmd[i];

		/* if this entry is a heading */
		if ( arg.lopt==nullptr && arg.sopt==nullptr && arg.call==nullptr && arg.args==nullptr)
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
			if ( arg.args==nullptr || ( arg.args[0]!='=' && strncmp(arg.args,"[=",2)!=0 ) )
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
		for ( i=0 ; i<sizeof(main_cmd)/sizeof(main_cmd[0]) ; i++ )
		{
			CMDARG arg = main_cmd[i];
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
