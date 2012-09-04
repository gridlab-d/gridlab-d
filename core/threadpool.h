/*  $Id: threadpool.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
 *
 *  Threadpool types, structures, and operations.
 *
 *  Author: Brandon Carpenter <brandon.carpenter@pnl.gov>
 *  Overhaul by DP Chassin 9/12
 */

#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include "platform.h"
#include "timestamp.h"
#include <pthread.h>

int processor_count(void);

/** Iterator data */
typedef void* MTICODE; /* main iterator processor exit code */
typedef void* MTIITEM; /* item passed to iterator calls */
typedef void* MTIDATA; /* data returned by iterator calls */

/** Iterator get function prototype 
	This function iterates through the list of items.
 **/
typedef MTIITEM (*MTIGETFN)(MTIITEM); /* arg is NULL for first, returns NULL for last */

/** Iterator call function prototype 
	This function calls the action on the item given the data
 **/
typedef MTIDATA (*MTICALLFN)(MTIITEM,MTIDATA);

/** Iterator return function prototype 
	This function compares to data object from item calls and returns a resulting data object
 **/
typedef MTIDATA (*MTIRETFN)(MTIDATA,MTIDATA);

/** Thread iterator data for each iterator **/
typedef struct s_mtiterator {
	pthread_t thread_id;		/**< pthread handle/id */
	int enabled;				/**< flag indicating thread is enabled */
	int active;					/**< flag indicating thread is active */
	void **target;				/**< array of target pointers */
	unsigned int n_targets;		/**< number of targets */
	MTIDATA data;				/**< iterator data */
} MTIINFO; /**< iterator data structure */

/** Thread iterator control block */
typedef struct s_mtinteratorlist {
	struct {
		pthread_cond_t cond;	/**< condition variable */	
		pthread_mutex_t mutex;	/**< mutex object */
		unsigned int counter;	/**< start/stop counter */
	} start, stop;				/**< start and stop cond/mutex */
	MTIDATA in_data;			/**< iterator input data */
	MTIDATA out_data;			/**< iterator output data */
	clock_t runtime;			/**< runtime clock */
	MTIINFO *iterator;		/**< list of iterators */
	unsigned int n_iterators;	/**< number of iterators */
	MTIGETFN get;				/**< iterator get function */
	MTICALLFN call;				/**< iterator call function */
	MTIRETFN rel;				/**< iterator return function */
} MTI; /** iterator list structure */

/** Multithread iterator initialization

	Call this function to create a multithread iterator (MTI).

	The get function must return the next (or first) item to iterate over.

	The call function must perform the iterated operation.

	The return function must combine the current return data with the newly returned data.

	@returns Pointer to MTI or NULL if failed.
 **/
MTI *mti_init(MTIGETFN get,		/**< iterator get function */
			  MTICALLFN call,	/**< iterator call function */
			  MTIRETFN ret);	/**< iterator return function */

/** Multithread iterator run

	Call this function to run a multithread iterator (MTI).

	@returns the return data.
 **/
MTIDATA mti_run(MTI *iterator,	/**< pointer return by mti_init */
				MTIDATA data);  /**< data to send to iterator call function */

#endif /* _THREADPOOL_H */

