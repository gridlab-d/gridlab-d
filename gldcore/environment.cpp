/** $Id: environment.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file environment.c
	@addtogroup environment Environment control
	@ingroup core
	
	Environment function manage the interface with the user environment.
	The default user environment is \p batch.  The \p matlab environment
	is currently under development is has only a minimal interface.

	@todo Finish the \p matlab environment (ticket #18)
 @{
 **/

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

#include "environment.h"
#include "exec.h"
#include "save.h"
#include "matlab.h"
#include "server.h"
#include "xcore.h"
#include "gui.h"

/** Starts the environment selected by the global_environment variable
 **/
STATUS environment_start(int argc, /**< the number of arguments to pass to the environment */
						 char *argv[]) /**< the arguments to pass to the environment */
{
	if (strcmp(global_environment,"batch")==0)
	{
		if (gui_get_root()) 
		{
			strcpy(global_environment,"gui");
			goto UseGui;
		}

		/* do the run */
		if (exec_start()==FAILED)
		{
			output_fatal("shutdown after simulation stopped prematurely");
			/*	TROUBLESHOOT
				The simulation stopped because an unexpected condition was encountered.
				This can be caused by a wide variety of things, but most often it is
				because one of the objects in the model could not be synchronized 
				propertly and its clock stopped.  This message usually follows a
				more specific message that indicates what caused the simulation to
				stop.
			 */
			if (global_dumpfile[0]!='\0')
			{
				if (!saveall(global_dumpfile))
					output_error("dump to '%s' failed", global_dumpfile);
					/* TROUBLESHOOT
						An attempt to create a dump file failed.  This message should be
						preceded by a more detailed message explaining why if failed.
						Follow the guidance for that message and try again.
					 */
				else
					output_debug("dump to '%s' complete", global_dumpfile);
			}
			return FAILED;
		}
		return SUCCESS;
	}
	else if (strcmp(global_environment,"matlab")==0)
	{
		output_verbose("starting Matlab");
		return matlab_startup(argc,argv);
	}
	else if (strcmp(global_environment,"server")==0)
	{
		// server only mode (no GUI)
		output_verbose("starting server");
		if (server_startup(argc,argv))
			return exec_start();
		else
			return FAILED;
	}
	else if (strcmp(global_environment,"html")==0)
	{
		// this mode simply dumps the html data to a file
		return gui_html_output_all();
	}
	else if (strcmp(global_environment,"gui")==0)
	{
UseGui:
		output_verbose("starting server");
		if (server_startup(argc,argv) && gui_startup(argc,argv))
		{
			STATUS result = exec_start();
			GUIENTITY *gui;
			if ( result!=SUCCESS ) return result;
			gui = gui_get_root();
			if ( gui==NULL ) return FAILED;
			if ( gui->hold )
				return server_join();
			else
				return FAILED;
		}
		else
			return FAILED;
	}
	else if (strcmp(global_environment,"X11")==0)
	{
#ifdef X11
		xstart();
		if (gui_get_root())
			gui_X11_start();
		return exec_start();
#else
		output_fatal("X11 not supported");
		return FAILED;
#endif
	}
	else
	{
		output_fatal("%s environment not recognized or supported",global_environment);
		/*	TROUBLESHOOT
			The environment specified isn't supported. Currently only
			the <b>batch</b> environment is normally supported, although 
			some builds can support other environments, such as <b>matlab</b>.
		*/
		return FAILED;
	}
}

/**@}*/
