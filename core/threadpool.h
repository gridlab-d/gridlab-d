/*  $Id: threadpool.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
 *
 *  Threadpool types, structures, and operations.
 *
 *  Author: Brandon Carpenter <brandon.carpenter@pnl.gov>
 */

#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include "list.h"

typedef const void * threadpool_t;
#define INVALID_THREADPOOL ((threadpool_t) NULL)

int processor_count(void);
threadpool_t tp_alloc(int *count, void (*run)(int thread, void *item));
void tp_exec(threadpool_t pool, LIST *list);
void tp_release(threadpool_t pool);

#endif /* _THREADPOOL_H */

