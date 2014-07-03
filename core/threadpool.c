/*  $Id: threadpool.c 4738 2014-07-03 00:55:39Z dchassin $
 *  Copyright (C) 2008 Battelle Memorial Institute
 *
 *  Threadpool implementation.
 *
 *  Author: Brandon Carpenter <brandon.carpenter@pnl.gov>
 *  Overhaul by DP Chassin Sept 2012
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#define _WIN32_WINNT 0x0400
#include <windows.h>
#endif

#include "globals.h"
#include "threadpool.h"

static int mti_debug_mode = 0;
int mti_debug(MTI *mti, char *fmt, ...)
{
	if ( mti_debug_mode )
	{
		int len = 0;
		char ts[64];
		va_list ptr;
		va_start(ptr,fmt);
		len += fprintf(stderr,"MTIDEBUG [%s] %s : ",convert_from_timestamp(global_clock,ts,sizeof(ts))?ts:"???", mti?mti->name:"(init)");
		len += vfprintf(stderr,fmt,ptr);
		len += fprintf(stderr,"%s","\n");
		va_end(ptr);
		return len;
	}
	return 0;
}

#ifdef HAVE_GET_NPROCS
#include <sys/sysinfo.h>
int processor_count(void) { return get_nprocs(); }
#elif defined(__MACH__)
#include <sys/param.h>
#include <sys/sysctl.h>
int processor_count(void)
{
	int count;
	size_t size = sizeof(count);
	if (sysctlbyname("hw.ncpu", &count, &size, NULL, 0))
		return 1;
	return count;
}
#else
int processor_count(void)
{
#ifdef WIN32
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
#else
	char *proc_count = getenv("NUMBER_OF_PROCESSORS");
	int count = proc_count ? atoi(proc_count) : 0;
	return count ? count : 1;
#endif /* WIN32 */
}
#endif /* HAVE_GET_NPROCS */

static MTICODE iterator_proc(MTIPROC *tp)
{
	MTI *mti = tp->mti;

	/* create the final result */
	MTIDATA final = mti->fn->set(NULL,NULL);

	/* loop as long as enabled */
	while ( tp->enabled )
	{
		size_t n;

		/* create the itermediate result */
		MTIDATA result = mti->fn->set(NULL,NULL);
		
		/* lock access to the start condition */
		pthread_mutex_lock(mti->start.lock);

		/* wait for the start condition to be satisfied */
		mti_debug(mti,"iterator %d waiting for start condition",tp->id);
		while ( mti->fn->compare(tp->data,mti->input)==0 )
			pthread_cond_wait(mti->start.cond,mti->start.lock);

		/* unlock access to the start condition */
		pthread_mutex_unlock(mti->start.lock);

		/* reset the final result */
		mti->fn->set(final,NULL);

		/* process each item in the list */
		mti_debug(mti,"iterator %d started", tp->id);
		tp->active = TRUE;
		for ( n=0 ; n<tp->n_items ; n++ )
		{
			/* reset the itermediate result */
			mti->fn->set(result,NULL);

			/* make the call */
			mti->fn->call(result,tp->item[n],mti->input);

			/* gather result */
			mti->fn->gather(final,result);
		}
		tp->active = FALSE;
		mti_debug(mti,"iterator %d completed", tp->id);

		/* update the current data */
		mti->fn->set(tp->data,mti->input);

		/* lock the stop condition */
		pthread_mutex_lock(mti->stop.lock);

		/* decrement the stop counter */
		mti->stop.count--;

		/* gather output */
		mti->fn->gather(mti->output,final);

		/* notify all of stop condition */
		pthread_cond_broadcast(mti->stop.cond);

		/* unlock stop condition */
		pthread_mutex_unlock(mti->stop.lock);

		/* free the itermediate result */
		free(result);
	}

	/* free the final result */
	free(final);

	/* exit */
	return (MTICODE)0;
}

MTI *mti_init(const char *name, MTIFUNCTIONS *fn, size_t minitems)
{
	pthread_cond_t new_cond = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t new_mutex = PTHREAD_MUTEX_INITIALIZER;
	MTI *mti = NULL;
	size_t nitems=0, items_per_process=0;
	MTIITEM item = NULL;
	char buffer[16];

	/* single threaded only possible */
	if ( global_threadcount==1 )
		return NULL;

	/* handle special debug mode */
	mti_debug_mode = global_getvar("mti_debug",buffer,sizeof(buffer))?atoi(buffer):0;
	mti_debug(mti,"MTI init started for %s", name);

	/* sizing pass */
	while ( (item=fn->get(item))!=NULL ) nitems++;
	if ( nitems==0 ) return NULL;

	/* construct MTI */
	mti = (MTI*)malloc(sizeof(MTI));
	if ( mti==NULL ) return NULL;
	mti->name = name;
	mti->start.cond = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
	memcpy(mti->start.cond,&new_cond,sizeof(new_cond));
	mti->start.lock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	memcpy(mti->start.lock,&new_mutex,sizeof(new_mutex));
	mti->start.count = 0;
	mti->stop.cond = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
	memcpy(mti->stop.cond,&new_cond,sizeof(new_cond));
	mti->stop.lock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	memcpy(mti->stop.lock,&new_mutex,sizeof(new_mutex));
	mti->stop.count = 0;
	mti->input = fn->set(NULL,NULL);
	mti->output = fn->set(NULL,NULL);
	mti->runtime = 0;
	mti->fn = fn;

	/* compute number of threads */
	mti->n_processes = global_threadcount;
	if ( nitems<mti->n_processes*minitems )
		mti->n_processes = nitems/minitems;
	if ( mti->n_processes==0 )
		mti->n_processes = 1;
	items_per_process = nitems/mti->n_processes;
	mti->n_processes = nitems/items_per_process;
	if ( mti->n_processes*items_per_process<nitems )
		mti->n_processes++;
	mti_debug(mti,"nitems = %d", nitems);
	mti_debug(mti,"nprocs = %d", mti->n_processes);
	mti_debug(mti,"items/proc = %d", items_per_process);

	/* construct process info if needed */
	if ( mti->n_processes>1 )
	{
		int p;

		/* get first item */
		MTIITEM item = fn->get(NULL);

		/* allocate/clear/init process list */
		mti_debug(mti,"preparing %d processes", mti->n_processes);
		mti->process = (MTIPROC*)malloc(sizeof(MTIPROC)*mti->n_processes);
		if ( mti->process==NULL )
		{
			output_error("mti_init memory allocation failed");
			/* TROUBLESHOOT
			   Memory allocation failed initialize a multithreaded
			   iterator.  Free up memory and try again.
			 */
			return NULL;
		}
		memset(mti->process,0,sizeof(MTIPROC)*mti->n_processes);
		for ( p=0 ; p<mti->n_processes ; p++ )
		{
			MTIPROC *proc = &mti->process[p];
			proc->mti = mti;
			proc->active = FALSE;
			proc->data = fn->set(NULL,NULL);
			proc->item = (MTIITEM*)malloc(sizeof(MTIITEM)*items_per_process);
			proc->n_items = 0;
			proc->id = p;

			/* index the items to be processed */
			while ( item!=NULL && proc->n_items<items_per_process )
			{
				proc->item[proc->n_items++] = item;
				item = fn->get(item);
			}

			/* create thread to handle the list */
			proc->enabled  = ( pthread_create(&proc->thread_id,NULL,(void*(*)(void*))iterator_proc,proc)==0 );
			mti_debug(mti,"proc=%d; enabled=%d, nitems=%d", p, proc->enabled, proc->n_items);
		}
	}
	else
		mti->process = NULL;	
	
	return mti;
}

int mti_run(MTIDATA result, MTI *mti, MTIDATA input)
{
	clock_t t0 = exec_clock();
	mti->fn->set(result,NULL);

	/* no update required */
	if ( mti->fn->reject(mti,input) )
	{
		mti_debug(mti,"no iteration update required");
		mti->fn->set(result,mti->output); 
		return 1;
	}

	/* singled threaded update */
	if ( mti->n_processes<2 )
	{
		mti_debug(mti,"iterating single threaded");
		return 0;
	}

	/* multi threaded update */
	else
	{
		mti_debug(mti,"starting %d iterators", mti->n_processes);

		/* lock access to stop condition */
		pthread_mutex_lock(mti->stop.lock);

		/* reset the stop condition */
		mti->stop.count = mti->n_processes;

		/* lock access to the start condition */
		pthread_mutex_lock(mti->start.lock);

		/* set start condition */
		mti->fn->set(mti->input,input);
		mti->fn->set(mti->output,NULL);

		/* broadcast start condition */
		pthread_cond_broadcast(mti->start.cond);

		/* unlock access to start condition */
		pthread_mutex_unlock(mti->start.lock);

		/* wait for stop condition */
		while ( mti->stop.count>0 )
			pthread_cond_wait(mti->stop.cond,mti->stop.lock);

		/* unlock access to stop condition */
		pthread_mutex_unlock(mti->stop.lock);

		/* gather result */
		mti->fn->gather(result,mti->output);
		mti_debug(mti,"%d iterators completed", mti->n_processes);
	}
	mti->runtime += exec_clock() - t0;
	return 1;
}
