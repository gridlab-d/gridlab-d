/** $Id: threadpool.h 4738 2014-07-03 00:55:39Z dchassin $
    Copyright (C) 2008 Battelle Memorial Institute
  
@file threadpool.h
@addtogroup mti Multi-threaded Iteration
@ingroup core

@author David P. Chassin (PNNL) 

Multi-threaded iterators (MTIs) can be used to convert single-threaded
for loops into iterators that take advantage of multiple core
when available.

The general scheme for implementing an MTI is as follows:

Step 1 - Define functions that provide the key operations required during
an MTI pass.  These are:

* Data get operation (#MTIGETFN) - This function gets items in the iteration list
* Iterator call (#MTICALLFN) - This function performs the iterated operation
* Data set operation (#MTISETFN) - This function set data for items
* Data compare operation (#MTICOMPFN) - This function compares data for items
* Data gather operation (#MTIGATHERFN) - This function gathers data for items
* Iterator reject test (#MTIREJECTFN) - This function allows for contingent rejection of iteration based on data

Step 2 - Initialize the iterator using #mti_init()

The iterator should be initialized only once and then it can be used repeatedly.
Aside from giving the iterator all the functions it should use, the iterator must
also be given a name (which is used by mti_debug output) and a minimum number of
items needed before a thread is created.

Step 3 - Call the iterator using #mti_run()

If the #mti_run() call succeeds or no iterations is needed, it returns 1.  If it
fails it returns 0 and a single threaded iteration should be performed by the caller
instead.

An example of how this is done is implemented in exec.c for commit_all().

@{**/

#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include "platform.h"
#include "timestamp.h"
#include <pthread.h>

typedef struct s_mtiteratorlist MTI;

/** Iterator data */
typedef void* MTICODE; /* main iterator processor exit code */
typedef void* MTIITEM; /* item passed to iterator calls */
typedef void* MTIDATA; /* data returned by iterator calls */

typedef struct s_mtifunctions
{
	/** Iterator get function prototype 
	    These functions iterate through the list of items.
	    @returns the next item in the iteration list (first if arg is null).
	 **/
	MTIITEM (*get)(MTIITEM); /* arg is NULL for first, returns NULL for last */
		
	/** Iterator call function prototype 
	    These functions call the action on the item given the second data.
	    The return data is copied into the first data.
	 **/
	void (*call)(MTIDATA,MTIITEM,MTIDATA);
	
	/** Data update function prototype 
	    These functions update the first data object given the second.
	    If the first is NULL, a new object is allocated.
	    If the second is NULL, the first is reset (e.g., zeroed).
	    Otherwise the second is copied to the first.
	    @returns the first value.
	 **/
	MTIDATA (*set)(MTIDATA,MTIDATA);
	
	/** Data compare object function prototype
	    These functions compare to data object and return -1,0,1 to indicate relation
	    -1 indicates the first value needs to be updated based on the second value
	    0 indicates the values are the same.
	    1 indicates no update to the first value is needed based on the second value
	    2 indicates no particularly important relation found
	    If a value is NULL, the comparison is made with a reset value (e.g., zero)
	 **/
	int (*compare)(MTIDATA,MTIDATA);
	
	/** Data gather function prototype
	    These functions determine whether to merge the second value into the first
	 **/
	void (*gather)(MTIDATA,MTIDATA);
	
	/** Data reject function prototype
	    These functions determine whether to reject the iteration based on the value given
	    @returns 0 if the iteration should proceed, 1 if the iteration is rejected
	 **/
	int (*reject)(MTI*,MTIDATA);
} MTIFUNCTIONS; /**< iteration function table */
	
/** Thread iterator data for each iterator **/
typedef struct s_mtiterator {
    unsigned int id;            /**< id given to iterator */
    MTI *mti;                   /**< pointer to MTI that controls this iterator */
    pthread_t thread_id;        /**< pthread handle/id */
    int enabled;                /**< flag indicating thread is enabled */
    int active;                 /**< flag indicating thread is active */
    MTIITEM *item;              /**< pointer to array of items */
    unsigned int n_items  ;     /**< number of items */
    MTIDATA data;               /**< iterator data */
} MTIPROC; /**< iterator process structure */

/** Thread iterator control block */
struct s_mtiteratorlist {
    const char *name;           /**< name given to iterator */
    struct {
        pthread_cond_t *cond;   /**< condition variable */    
        pthread_mutex_t *lock;  /**< mutex object */
        unsigned int count;     /**< start/stop counter */
    } start, stop;              /**< start and stop cond/mutex */
    MTIDATA input;              /**< iterator input data */
    MTIDATA output;             /**< iterator output data */
    MTIFUNCTIONS *fn;		/**< function table */
    clock_t runtime;            /**< runtime clock */
    unsigned int n_processes;   /**< number of iterators */
    MTIPROC *process;           /**< list of iterator processes */
}; /** iterator list structure */

#ifdef __cplusplus
extern "C" {
#endif

/** Multithread iterator initialization

    Call this function to create a multithread iterator (MTI).

    The get function must return the next (or first) item to iterate over.

    The call function must perform the iterated operation.

    The return function must combine the current return data with the newly returned data.

    @returns Pointer to MTI or NULL if failed.
 **/
MTI *mti_init(const char *name, /**< name of iterator */
              MTIFUNCTIONS *fns, /**< iterator function table */
              size_t minitems); /**< minimum number of items per thread */

/** Multithread iterator run

    Call this function to run a multithread iterator (MTI).

    @returns 1 on success, 0 if single threaded iterator must be used
 **/
int mti_run(MTIDATA result,   /**< result of iterator */
            MTI *iterator,    /**< pointer return by mti_init */
            MTIDATA input);   /**< data to send to iterator call function */

int processor_count(void);
#ifdef __cplusplus
}
#endif

#endif /**@} _THREADPOOL_H */

