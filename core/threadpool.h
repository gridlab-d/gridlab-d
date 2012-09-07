/*  $Id: threadpool.h 1182 2008-12-22 22:08:36Z dchassin $
 *  Copyright (C) 2008 Battelle Memorial Institute
 *
 *  @file threadpool.h
 *  @addtogroup mti Multi-threaded Iteration
 *  @ingroup core
 *  Threadpool types, structures, and operations.
 *
 *  Author: Brandon Carpenter <brandon.carpenter@pnl.gov>
 *  Overhaul by DP Chassin Sept 2012
 *
 * @{
 */

#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include "platform.h"
#include "timestamp.h"
#include <pthread.h>

int processor_count(void);

typedef struct s_mtiteratorlist MTI;

/** Iterator data */
typedef void* MTICODE; /* main iterator processor exit code */
typedef void* MTIITEM; /* item passed to iterator calls */
typedef void* MTIDATA; /* data returned by iterator calls */

/** Iterator get function prototype 
    These functions iterate through the list of items.
    @returns the next item in the iteration list (first if arg is null).
 **/
typedef MTIITEM (*MTIGETFN)(MTIITEM); /* arg is NULL for first, returns NULL for last */

/** Iterator call function prototype 
    These functions call the action on the item given the second data.
    The return data is copied into the first data.
 **/
typedef void (*MTICALLFN)(MTIDATA,MTIITEM,MTIDATA);

/** Data update function prototype 
    These functions update the first data object given the second.
    If the first is NULL, a new object is allocated.
    If the second is NULL, the first is reset (e.g., zeroed).
    Otherwise the second is copied to the first.
    @returns the first value.
 **/
typedef MTIDATA (*MTISETFN)(MTIDATA,MTIDATA);

/** Data compare object function prototype
    These functions compare to data object and return -1,0,1 to indicate relation
    -1 indicates the first value needs to be updated based on the second value
    0 indicates the values are the same.
    1 indicates no update to the first value is needed based on the second value
    2 indicates no particularly important relation found
    If a value is NULL, the comparison is made with a reset value (e.g., zero)
 **/
typedef int (*MTICOMPFN)(MTIDATA,MTIDATA);

/** Data gather function prototype
    These functions determine whether to merge the second value into the first
 **/
typedef void (*MTIGATHERFN)(MTIDATA,MTIDATA);

/** Data reject function prototype
    These functions determine whether to reject the iteration based on the value given
    @returns 0 if the iteration should proceed, 1 if the iteration is rejected
 **/
typedef int (*MTIREJECTFN)(MTI*,MTIDATA);

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
    clock_t runtime;            /**< runtime clock */
    MTIPROC *process;           /**< list of iterator processes */
    unsigned int n_processes;   /**< number of iterators */
    MTIGETFN get;               /**< iterator get function */
    MTICALLFN call;             /**< iterator call function */
    MTISETFN set;               /**< data update function */
    MTICOMPFN compare;          /**< data compare function */
    MTIGATHERFN gather;         /**< data gather function */
    MTIREJECTFN reject;         /**< data reject function */
}; /** iterator list structure */

/** Multithread iterator initialization

    Call this function to create a multithread iterator (MTI).

    The get function must return the next (or first) item to iterate over.

    The call function must perform the iterated operation.

    The return function must combine the current return data with the newly returned data.

    @returns Pointer to MTI or NULL if failed.
 **/
MTI *mti_init(const char *name, /**< name of iterator */
              MTIGETFN get,     /**< iterator get function */
              MTICALLFN call,   /**< iterator call function */
              MTISETFN ret,     /**< data update function */
              MTICOMPFN comp,   /**< data compare function */
              MTIGATHERFN gather, /**< data gather function */
              MTIREJECTFN reject, /**< iteration reject function */
              size_t minitems); /**< minimum number of items per thread */

/** Multithread iterator run

    Call this function to run a multithread iterator (MTI).

    @returns 1 on success, 0 if single threaded iterator must be used
 **/
int mti_run(MTIDATA result,   /**< result of iterator */
            MTI *iterator,    /**< pointer return by mti_init */
            MTIDATA input);   /**< data to send to iterator call function */

#endif /* _THREADPOOL_H  @} */

