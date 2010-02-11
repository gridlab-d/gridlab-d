/** $Id: main.c 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file main.c
	@author David P. Chassin

 @{
 **/
#define _MAIN_C

#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <direct.h>
#include <process.h>
#else
#include <unistd.h>
#endif

#include "globals.h"
#include "legal.h"
#include "cmdarg.h"
#include "module.h"
#include "output.h"
#include "environment.h"
#include "test.h"
#include "random.h"
#include "realtime.h"
#include "save.h"
#include "local.h"
#include "exec.h"
#include "kml.h"

#if defined WIN32 && _DEBUG 
/** Implements a pause on exit capability for Windows consoles
 **/
void pause_at_exit(void) 
{
	if (global_pauseatexit)
		system("pause");
}
#endif

void delete_pidfile(void)
{
	unlink(global_pidfile);
}

/** The main entry point of GridLAB-D
 Exit codes
 - \b 0 run completed ok
 - \b 1 command-line processor stopped
 - \b 2 environment startup failed
 - \b 3 test procedure failed
 - \b 4 user rejects conditions of use
 - \b 5 simulation stopped before completing
 - \b 6 initialization failed
 **/
int main(int argc, /**< the number entries on command-line argument list \p argv */
		 char *argv[]) /**< a list of pointers to the command-line arguments */
{
	int rv = 0;
	time_t t_load = time(NULL);
	global_process_id = getpid();
#if defined WIN32 && _DEBUG 
	atexit(pause_at_exit);
#endif
	/* main initialization */
	if (!output_init(argc,argv) || !exec_init())
		exit(6);

	/* process command line arguments */
	if (cmdarg_load(argc,argv)==FAILED)
	{
		output_fatal("shutdown after command line rejected");
		/*	TROUBLESHOOT
			The command line is not valid and the system did not
			complete its startup procedure.  Correct the problem
			with the command line and try again.
		 */
		exit(1);
	}

	/* setup the random number generator */
	random_init();

	/* pidfile */
	if (strcmp(global_pidfile,"")!=0)
	{
		FILE *fp = fopen(global_pidfile,"w");
		if (fp==NULL)
		{
			output_fatal("unable to create pidfile '%s'", global_pidfile);
			/*	TROUBLESHOOT
				The system must allow creation of the process id file at
				the location indicated in the message.  Create and/or
				modify access rights to the path for that file and try again.
			 */
			exit(1);
		}
#ifdef WIN32
#define getpid _getpid
#endif
		fprintf(fp,"%d\n",getpid());
		output_verbose("process id %d written to %s", getpid(), global_pidfile);
		fclose(fp);
		atexit(delete_pidfile);
	}

	/* do legal stuff */
#ifdef LEGAL_NOTICE
	if (strcmp(global_pidfile,"")==0 && legal_notice()==FAILED)
		exit(4);
#endif

	/* set up the test */
	if (global_test_mode)
	{
#ifndef _NO_CPPUNIT
		output_message("Entering test mode");
		if (test_start(argc,argv)==FAILED)
		{
			output_fatal("shutdown after startup test failed");
			/*	TROUBLESHOOT
				A self-test procedure failed and the system stopped.
				Check the output of the test stream and correct the 
				indicated problem.  If the test stream is not redirected
				so as to save the output, try using the <b>--redirect</b>
				command line option.
			 */
			exit(3);
		}
		exit(0); /* There is no environment to speak of, so exit. */
#else
		output_message("Unit Tests not enabled.  Recompile with _NO_CPPUNIT unset");
#endif
	}
	
	/* start the processing environment */
	output_verbose("load time: %d sec", time(NULL) - t_load);
	output_verbose("starting up %s environment", global_environment);
	if (environment_start(argc,argv)==FAILED)
	{
		output_fatal("environment startup failed: %s", strerror(errno));
		/*	TROUBLESHOOT
			The requested environment could not be started.  This usually
			follows a more specific message regarding the startup problem.
			Follow the recommendation for the indicated problem.
		 */
		rv = 2;
	}

	/* post process the test */
	if (global_test_mode)
	{
#ifndef _NO_CPPUNIT
		output_message("Exiting test mode");
		if (test_end(argc,argv)==FAILED)
		{
			output_error("shutdown after end test failed");
			exit(3);
		}
#endif
	}

	/* save the model */
	if (strcmp(global_savefile,"")!=0)
	{
		if (saveall(global_savefile)==FAILED)
			output_error("save to '%s' failed", global_savefile);
	}

	/* do module dumps */
	if (global_dumpall!=FALSE)
	{
		output_verbose("dumping module data");
		module_dumpall();
	}

	/* KML output */
	if (strcmp(global_kmlfile,"")!=0)
		kml_dump(global_kmlfile);

	/* wrap up */
	output_verbose("shutdown complete");

	/* profile results */
	if (global_profiler)
		class_profiles();

	/* restore locale */
	locale_pop();

	/* compute elapsed runtime */
	output_verbose("elapsed runtime %d seconds", realtime_runtime());

	exit(rv);
}

/** @} **/
