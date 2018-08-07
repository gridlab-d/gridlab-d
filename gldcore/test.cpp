/* test.c
 * Copyright (C) 2008 Battelle Memorial Institute
 * Integrated system testing routines.  Start is called before executive runs, end after executive exits.
 */

#include <stdlib.h>

#include "globals.h"
#include "cmdarg.h"
#include "module.h"
#include "output.h"
#include "save.h"
#include "find.h"
#include "test.h"
#include "aggregate.h"

typedef struct s_testlist {
	char name[64];
	TESTFUNCTION call;
	int enabled;
	struct s_testlist *next;
} TESTLIST;
static TESTLIST test_list[] = {
	{"dst",			timestamp_test,		0, test_list+1},
	{"rand",		random_test,		0, test_list+2},
	{"units",		unit_test,			0, test_list+3},
	{"schedule",	schedule_test,		0, test_list+4},
	{"loadshape",	loadshape_test,		0, test_list+5},
	{"enduse",		enduse_test,		0, test_list+6},
	{"lock",		test_lock,			0, NULL}, /* last test in list has no next */
	/* add new core test routines before this line */
}, *last_test = test_list+sizeof(test_list)/sizeof(test_list[0])-1;

int test_register(char *name, TESTFUNCTION call)
{
	TESTLIST *item = (TESTLIST*)malloc(sizeof(TESTLIST));
	if ( item==NULL )
	{
		output_error("test_register(char *name='%s', TESTFUNCTION call=%p): memory allocation failed", name, call);
		return FAILED;
	}
	last_test->next = item;
	strncpy(item->name,name,sizeof(item->name));
	item->call = call;
	item->enabled = 0;
	item->next = NULL;
	last_test = item;
	return SUCCESS;
}

int test_request(char *name)
{
	TESTLIST *item;
	MODULE *mod;

	output_verbose("running test '%s'...",name);

	/* try already list ones */
	for ( item=test_list ; item!=NULL ; item=item->next )
	{
		if ( strcmp(item->name,name)==0 )
		{
			item->enabled = 1;
			return SUCCESS;
		}
	}

	/* try module test */
	if ( (mod=module_load(name,0,NULL))!=NULL )
	{
		if ( mod->test!=NULL )
			mod->test(0,NULL);
		else
			output_warning("module '%s' does not implement a test routine", name);
		return SUCCESS;
	}

	return FAILED;
}

int test_exec(void)
{
	TESTLIST *item;
	for ( item=test_list ; item!=NULL ; item=item->next )
	{
		if ( item->enabled!=0 )
		{
			item->call();
		}
	}
	return SUCCESS;
}


/***********************************************************************
 * MEMORY LOCK TEST
 */
#include "lock.h"
#include "pthread.h"
#include "exec.h"
#define TESTCOUNT (100000000/global_threadcount)

static volatile unsigned int *count = NULL;
static volatile unsigned int total = 0;
static unsigned int key = 0;
static volatile int done = 0;

static void *test_lock_proc(void *ptr)
{
	int id = *(int*)ptr;
	int m;
	output_test("thread %d created ok", (unsigned int)id);
	for ( m=0 ; m<TESTCOUNT ; m++ )
	{
		wlock(&key);
		count[id]++;
		total++;
		wunlock(&key);
	}
	wlock(&key);
	done++;
	wunlock(&key);
	output_test("thread %d done ok", (unsigned int)id);
	return (void*)0;
}

int test_lock(void)
{
	int n, sum=0;

	count = (unsigned int*)malloc(sizeof(unsigned int*)*global_threadcount);
	if ( !count )
	{
		output_test("memory allocation failed");
		return FAILED;
	}
	
	output_test("*** Begin memory locking test for %d threads", global_threadcount);
	wlock(&key);
	for ( n=0 ; n<global_threadcount ; n++ )
	{
		pthread_t pt;
		count[n] = 0;
		if ( pthread_create(&pt,NULL,test_lock_proc,(void*)&n)!=0 )
		{
			output_test("thread creation failed");
			return FAILED;
		}
	}
	wunlock(&key);
	global_suppress_repeat_messages = 0;
	for ( n=0 ; n<global_threadcount ; n++ )
		output_raw("THREAD %2d  ", n);
	output_message("  TOTAL     ERRORS");
	for ( n=0 ; n<global_threadcount ; n++ )
		output_raw("---------- ", n);
	output_message("---------- --------");
	while ( done<global_threadcount )
	{
		int c[256], t, s=0;
		exec_sleep(100000);
		output_raw("\r");
		rlock(&key);
		for ( n=0 ; n<global_threadcount ; n++ )
			s += (c[n]=count[n]);
		t = total;
		runlock(&key);
		for ( n=0 ; n<global_threadcount ; n++ )
			output_raw("%10d ",c[n]);
		output_raw("%10d %8d",t,t-s);
	}
	output_message("");
	for ( n=0 ; n<global_threadcount ; n++ )
	{
		output_test("thread %d count = %d", n, count[n]);
		sum+=count[n];
	}
	output_test("total count = %d", total);
	if ( sum!=total )
		output_test("TEST FAILED");
	else
		output_test("Last key = %d", key);
	output_test("*** End memory locking test", global_threadcount);
	return SUCCESS;
}

