/*  $Id: threadpool.c 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
 *
 *  Threadpool implementation.
 *
 *  Author: Brandon Carpenter <brandon.carpenter@pnl.gov>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "threadpool.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

struct threadpool;

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <process.h>
#include <limits.h>

typedef struct {
	CRITICAL_SECTION critsec;
	HANDLE exec_sem, done_event;
} sync_t;

static __inline void sync_init(struct threadpool *tp)
{
	sync_t *sync = (sync_t *) tp;

	InitializeCriticalSection(&sync->critsec);
	sync->exec_sem = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
	sync->done_event = CreateEvent(NULL, TRUE, FALSE, NULL);
}

static __inline void sync_done(struct threadpool *tp)
{
	sync_t *sync = (sync_t *) tp;

	DeleteCriticalSection(&sync->critsec);
	CloseHandle(sync->exec_sem);
	CloseHandle(sync->done_event);
}

static __inline void fast_enter_critical_section(struct threadpool *tp)
{
	sync_t *sync = (sync_t *) tp;

	while (!TryEnterCriticalSection(&sync->critsec))
		SwitchToThread();
}

static __inline void wait_for_exec(struct threadpool *tp)
{
	sync_t *sync = (sync_t *) tp;

	LeaveCriticalSection(&sync->critsec);
	WaitForSingleObject(sync->exec_sem, INFINITE);
	EnterCriticalSection(&sync->critsec);
}

static __inline void wait_for_completion(struct threadpool *tp)
{
	sync_t *sync = (sync_t *) tp;

	ResetEvent(sync->done_event);
	LeaveCriticalSection(&sync->critsec);
	WaitForSingleObject(sync->done_event, INFINITE);
	EnterCriticalSection(&sync->critsec);
}

#define create_thread(proc, arg) (_beginthread(proc, 0, arg) == -1)
#define enter_critical_section(tp) EnterCriticalSection(&((sync_t *) tp)->critsec)
#define leave_critical_section(tp) LeaveCriticalSection(&((sync_t *) tp)->critsec)
#define signal_exec(tp) ReleaseSemaphore(((sync_t *) tp)->exec_sem, min(tp->wait_count, tp->thread_count), NULL)
#define signal_completion(tp) SetEvent(((sync_t *) tp)->done_event)

#elif defined(HAVE_PTHREAD)

#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <pthread.h>

#ifdef __MACH__   /* BSD implementation uses different yield function */
#define pthread_yield pthread_yield_np
#endif

typedef struct {
	pthread_mutex_t mutex;
	pthread_cond_t exec_cond, done_cond;
} sync_t;

static __inline int create_thread(void * (*proc)(void *), void *arg)
{
	pthread_t t;
	return pthread_create(&t, NULL, proc, arg);
}

static __inline void sync_init(struct threadpool *tp)
{
	sync_t *sync = (sync_t *) tp;

	pthread_mutex_init(&sync->mutex, NULL);
	pthread_cond_init(&sync->exec_cond, NULL);
	pthread_cond_init(&sync->done_cond, NULL);
}

static __inline void sync_done(struct threadpool *tp)
{
	sync_t *sync = (sync_t *) tp;

	pthread_mutex_destroy(&sync->mutex);
	pthread_cond_destroy(&sync->exec_cond);
	pthread_cond_destroy(&sync->done_cond);
}

static __inline void fast_enter_critical_section(struct threadpool *tp)
{
	sync_t *sync = (sync_t *) tp;

	while (pthread_mutex_trylock(&sync->mutex))
		pthread_yield();
}

#define enter_critical_section(tp) pthread_mutex_lock(&((sync_t *) tp)->mutex)
#define leave_critical_section(tp) pthread_mutex_unlock(&((sync_t *) tp)->mutex)
#define wait_for_exec(tp) pthread_cond_wait(&((sync_t *) tp)->exec_cond, \
		&((sync_t *) tp)->mutex)
#define signal_exec(tp) pthread_cond_broadcast(&((sync_t *) tp)->exec_cond)
#define wait_for_completion(tp) pthread_cond_wait(&((sync_t *) tp)->done_cond, \
		&((sync_t *) tp)->mutex)
#define signal_completion(tp) pthread_cond_signal(&((sync_t *) tp)->done_cond)
#else

typedef char sync_t;
#define create_thread(proc, arg) (~0)
#define sync_init(tp)
#define sync_done(tp)
#define fast_enter_critical_section(tp)
#define enter_critical_section(tp)
#define leave_critical_section(tp)
#define wait_for_exec(tp)
#define signal_exec(tp)
#define wait_for_completion(tp)
#define signal_completion(tp)

#endif /* WIN32 */


struct threadpool {
	sync_t sync;
	int thread_count, thread_index, thread_die, wait_count;
	void (*run)(int thread, void *item);
	LISTITEM *list;
};


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


static __inline void exec_loop(struct threadpool *tp, int thread)
{
	int i;
	LISTITEM *item;

	leave_critical_section(tp);
	item = tp->list;
	for (i = 0; i < thread && item; i++)
		item = item->next;
	while (item) {
		tp->run(thread, item->data);
		for (i = 0; i <= tp->thread_count && item; i++)
			item = item->next;
	}
	enter_critical_section(tp);
}


#ifdef HAVE_PTHREAD
static void *thread_proc(void *arg)
#else
static int thread_proc(void *arg)
#endif /* HAVE_PTHREAD */
{
	struct threadpool *tp = (struct threadpool *) arg;
	int index;

	enter_critical_section(tp);
	index = ++tp->thread_index;

	for(;;) {
		tp->wait_count--;
		if (!tp->wait_count)
			signal_completion(tp);
		wait_for_exec(tp);
		if (tp->thread_die)
			break;
		exec_loop(tp, index);
	}

	if (!(--tp->wait_count))
		signal_completion(tp);
	leave_critical_section(tp);

	return 0;
}


threadpool_t tp_alloc(int *count, void (*run)(int thread, void *item))
{
	int i;
	struct threadpool *tp =
				(struct threadpool *) malloc(sizeof(struct threadpool));

	if (!tp)
		return INVALID_THREADPOOL;

	memset(tp, 0, sizeof(struct threadpool));
	sync_init(tp);
	tp->run = run;

	enter_critical_section(tp);

	for (i = 1; i < *count; i++) {
		if (!create_thread(thread_proc, tp))
			tp->thread_count++;
	}

	tp->wait_count = tp->thread_count;
	if (tp->wait_count)
		wait_for_completion(tp);
	*count = tp->thread_count + 1;
	leave_critical_section(tp);

	return tp;
}


void tp_exec(threadpool_t pool, LIST *list)
{
	struct threadpool *tp = (struct threadpool *) pool;

	enter_critical_section(tp);
	tp->list = list->first;
	/* tp->wait_count = list->size; */
	tp->wait_count = tp->thread_count;
	signal_exec(tp);
	exec_loop(tp, 0);
	if (tp->wait_count)
		wait_for_completion(tp);
	leave_critical_section(tp);
}


void tp_release(threadpool_t pool)
{
	struct threadpool *tp = (struct threadpool *) pool;

	enter_critical_section(tp);
	tp->thread_die = ~0;
	tp->list = NULL;
	tp->wait_count = tp->thread_count;
	signal_exec(tp);
	if (tp->wait_count)
		wait_for_completion(tp);
	leave_critical_section(tp);

	sync_done(tp);
	free(tp);
}


#ifdef STANDALONE_TEST

#include <stdio.h>
#include <errno.h>

static void my_run(int thread, void *item)
{
	printf("%s  %d\n", (char *) item, thread);
	system((char *) item);
}

int main(int argc, char *argv[])
{
	LISTITEM *i, items[] = {
		{ (void *) "sleep 0.9 #  0", NULL, NULL },
		{ (void *) "sleep 0.2 #  1", NULL, NULL },
		{ (void *) "sleep 0.3 #  2", NULL, NULL },
		{ (void *) "sleep 0.8 #  3", NULL, NULL },
		{ (void *) "sleep 0.4 #  4", NULL, NULL },
		{ (void *) "sleep 0.1 #  5", NULL, NULL },
		{ (void *) "sleep 0.8 #  6", NULL, NULL },
		{ (void *) "sleep 0.7 #  7", NULL, NULL },
		{ (void *) "sleep 0.6 #  8", NULL, NULL },
		{ (void *) "sleep 1   #  9", NULL, NULL },
		{ (void *) "sleep 3   # 10", NULL, NULL },
		{ (void *) "sleep 2   # 11", NULL, NULL },
		{ (void *) "sleep 0.5 # 12", NULL, NULL },
		{ (void *) "sleep 2   # 13", NULL, NULL },
		{ (void *) "sleep 1   # 14", NULL, NULL },
		{ NULL, NULL, NULL},
	};
	LIST list = {0, NULL, NULL};
	int thread_count = 4;
	struct threadpool *tp = (struct threadpool *) tp_alloc(&thread_count, my_run);

	if (tp == INVALID_THREADPOOL) {
		printf("%s: error allocating threadpool: %s\n", argv[0], strerror(errno));
		exit(1);
	}
	printf("allocated %d threads\n", thread_count);

	if (items && items[0].data) {
		list.first = items;
		for (i = items; i && (i + 1)->data; i++) {
			i->next = i + 1;
			list.size++;
		}
		i->next = NULL;
		list.last = i;
		list.size++;
	}

	tp_exec(tp, &list);
	tp_release(tp);

	return 0;
}

#endif /* STANDALONE_TEST */
