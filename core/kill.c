/** $Id: kill.c 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
   @file kill.c
   @addtogroup exec
   This program provide the same kill functionality that is available to Linux versions

 @{
 **/

#ifdef WIN32
#include "windows.h"
#include "output.h"
#include "signal.h"
#include "process.h"

/* KILLONLY is undefined when building the kill program, 
   and defined when compiling with the main GridLAB-D 
   core modules
 */
#ifndef KILLONLY

#include "gridlabd.h"

static int handler_stop = 0;
void kill_stophandler(void)
{
	handler_stop = 1;
}

void msghandler(void *param)
{
	char name[32];
	HANDLE hEvent;
	unsigned int sig = (unsigned int)(int64)param;
	unsigned int pid = _getpid();
	sprintf(name,"gridlabd.%d.%d",pid,sig);
	hEvent = CreateEventA(NULL,TRUE,FALSE,name);
	output_verbose("creating gridlabd signal handler %d for process %d",sig,pid);
	while (WaitForSingleObject(hEvent,INFINITE)==WAIT_OBJECT_0)
	{
		output_verbose("windows signal handler activated");
		raise(sig);
		ResetEvent(hEvent);
	}
}

void kill_starthandler(void)
{
	if (_beginthread(&msghandler, 0, (void*)SIGINT)==1 || _beginthread(&msghandler, 0, (void*)SIGTERM)==1)
		output_error("kill handler failed to start");
	else
		output_verbose("windows message signal handlers started");
}
#else
#define output_error printf
#define output_verbose
#endif

/** Send a kill signal to a windows version of GridLAB-D
 **/
int kill(unsigned short pid,	/**< the window process id */
		 int sig)				/**< the signal id (see signal.h) */
{
	char name[32];
	HANDLE hEvent;
	sprintf(name,"gridlabd.%d.%d",(int)pid,sig);
	hEvent = OpenEventA(EVENT_MODIFY_STATE,FALSE,name);
	if (hEvent==NULL)
	{
		output_error("unable to signal gridlabd process %d with signal %d (error %d)", pid, sig, GetLastError());
		return 0;
	}
	else
	{
		SetEvent(hEvent);
		output_verbose("signal %d sent to gridlabd process %d", sig, pid);
		return 1;
	}
}
#endif

/** @} */
