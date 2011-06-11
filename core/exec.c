/** $Id: exec.c 1188 2009-01-02 21:51:07Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file exec.c
	@addtogroup exec Main execution loop
	@ingroup core
	
	The main execution loop sets up the main simulation, initializes the
	objects, and runs the simulation until it either settles to equilibrium
	or runs into a problem.  It also takes care multicore/multiprocessor
	parallelism when possible.  Objects of the same rank will be synchronized
	simultaneously, resources permitting.

	The main processing loop calls each object passing to it a TIMESTAMP
	indicating the desired synchronization time.  The sync() call attempts to
	advance the object's internal clock to the time indicated, and if successful it
	returns the time of the next expected change in the object's state.  An
	object state change is one which requires the equilibrium equations of
	the object to be updated.  When an object's state changes, all the other
	objects in the simulator are given an opportunity to consider the change
	and possibly alter the time of their next state change.  The core
	continues calling objects, advancing the global clock when
	necessary, and continuing in this way until all objects indicate that
	no further state changes are expected.  This is the equilibrium condition
	and the simulation consequently ends.

	@future [Chassin Oct'07]

	There is some value in exploring whether it is necessary to update all
	objects when a particular objects implements a state change.  The idea is
	based on the fact that updates propagate through the model based on known
	relations, such at the parent-child relation or the link-node relation.
	Consequently, it should obvious that unless a value in a related object
	has changed, there can be no significant change to an object that hasn't reached
	it's declared update time.  Thus only the object that "won" the next update
	time and those that are immediately related to it need be updated.  This 
	change could result in a very significant improvement in performance,
	particularly in models with many lightly coupled objects. 

 @{
 **/

#include <signal.h>
#include <ctype.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#include <winbase.h>
#include <direct.h>
#include <sys/timeb.h>
#else
#include <unistd.h>
#endif
#include "output.h"
#include "exec.h"
#include "class.h"
#include "convert.h"
#include "object.h"
#include "index.h"
#include "realtime.h"
#include "module.h"
#include "threadpool.h"
#include "debug.h"
#include "exception.h"
#include "random.h"	
#include "local.h"
#include "schedule.h"
#include "loadshape.h"
#include "enduse.h"
#include "globals.h"
#include "math.h"
#include "time.h"
#include "lock.h"
#include "stream.h"

#include "pthread.h"

/** The main system initialization sequence
	@return 1 on success, 0 on failure
 **/

int exec_init()
{
#if 0
#ifdef WIN32
	char glpathvar[1024];
#endif
#endif

	size_t glpathlen=0;
	/* setup clocks */
	global_starttime = realtime_now();
	timestamp_set_tz(NULL);

	/* determine current working directory */
	getcwd(global_workdir,1024);

	/* save locale for simulation */
	locale_push();

#if 0 /* isn't cooperating for strange reasons -mh */
#ifdef WIN32
	glpathlen=strlen("GLPATH=");
	sprintf(glpathvar, "GLPATH=");
	ExpandEnvironmentStrings(getenv("GLPATH"), glpathvar+glpathlen, (DWORD)(1024-glpathlen));
#endif
#endif

	if (global_init()==FAILED)
		return 0;
	return 1;
}

//sjin: GetMachineCycleCount
/*int mc_start_time;
int mc_end_time;
int GetMachineCycleCount(void)
{      
   __int64 cycles;
   _asm rdtsc; // won't work on 486 or below - only pentium or above

   _asm lea ebx,cycles;
   _asm mov [ebx],eax;
   _asm mov [ebx+4],edx;
   return cycles;
}*/
clock_t cstart, cend;

#ifndef _MAX_PATH
#define _MAX_PATH 1024
#endif

#define PASSINIT(p) (p % 2 ? ranks[p]->first_used : ranks[p]->last_used)
#define PASSCMP(i, p) (p % 2 ? i <= ranks[p]->last_used : i >= ranks[p]->first_used)
#define PASSINC(p) (p % 2 ? 1 : -1)

static struct thread_data *thread_data = NULL;
static INDEX **ranks = NULL;
const PASSCONFIG passtype[] = {PC_PRETOPDOWN, PC_BOTTOMUP, PC_POSTTOPDOWN};
static unsigned int pass;
int iteration_counter = 0;   /* number of redos completed */

#ifndef NOLOCKS
int64 lock_count = 0, lock_spin = 0;
#endif

extern int stop_now;

//sjin: struct for pthread_create arguments
struct arg_data {
	int thread;
	void *item;
	int incr;
};
struct arg_data arg_data_array[2];

INDEX **exec_getranks(void)
{
	return ranks;
}

static STATUS setup_ranks(void)
{
	OBJECT *obj;
	int i;
	static INDEX *passlist[] = {NULL,NULL,NULL,NULL}; /* extra NULL marks the end of the list */

	/* create index object */
	ranks = passlist;
	ranks[0] = index_create(0,10);
	ranks[1] = index_create(0,10);
	ranks[2] = index_create(0,10);

	/* build the ranks of each pass type */
	for (i=0; i<sizeof(passtype)/sizeof(passtype[0]); i++)
	{
		if (ranks[i]==NULL)
			return FAILED;

		/* add every object to index based on rank */
		for (obj=object_get_first(); obj!=NULL; obj=object_get_next(obj))
		{
			/* ignore objects that don't use this passconfig */
			if ((obj->oclass->passconfig&passtype[i])==0)
				continue;

			/* add this object to the ranks for this passconfig */
			if (index_insert(ranks[i],obj,obj->rank)==FAILED) 
				return FAILED;
			//sjin: print out obj id, pass, rank information
			//else 
			//	printf("obj[%d]: pass = %d, rank = %d\n", obj->id, passtype[i], obj->rank);
		}

		if (global_debug_mode==0 && global_nolocks==0)

			/* shuffle the objects in the index */
			index_shuffle(ranks[i]);
	}

	return SUCCESS;
}

char *simtime(void)
{
	static char buffer[64];
	return convert_from_timestamp(global_clock,buffer,sizeof(buffer))>0?buffer:"(invalid)";
}

static STATUS show_progress(void)
{
	output_progress();
	/* reschedule report */
	realtime_schedule_event(realtime_now()+1,show_progress);
	return SUCCESS;
}

/***********************************************************************/
/* CHECKPOINTS (DPC Apr 2011) */

void do_checkpoint(void)
{
	/* last checkpoint value */
	static TIMESTAMP last_checkpoint = 0;
	TIMESTAMP now = 0;

	/* check point type selection */
	switch (global_checkpoint_type) {

	/* wallclock checkpoint interval */
	case CPT_WALL: 

		/* checkpoint based on wall time */
		now = time(NULL);

		/* default checkpoint for WALL */
		if ( global_checkpoint_interval==0 )
			global_checkpoint_interval = 3600;

		break;
	
		/* simulation checkpoint interval */
	case CPT_SIM: 

		/* checkpoint based on sim time */
		now = global_clock;

		/* default checkpoint for SIM */
		if ( global_checkpoint_interval==0 )
			global_checkpoint_interval = 86400;

		break;

	/* no checkpoints used */
	case CPT_NONE: 
		now = 0;
		break;
	}

	/* checkpoint may be needed */
	if ( now > 0 )
	{
		/* initial value of last checkpoint */
		if ( last_checkpoint==0 )
			last_checkpoint = now;

		/* checkpoint time lapsed */
		if ( last_checkpoint + global_checkpoint_interval <= now )
		{
			static char fn[1024] = "";
			FILE *fp = NULL;

			/* default checkpoint filename */
			if ( strcmp(global_checkpoint_file,"")==0 )
			{
				char *ext;

				/* use the model name by default */
				strcpy(global_checkpoint_file, global_modelname);
				ext = strrchr(global_checkpoint_file,'.');

				/* trim off the extension, if any */
				if ( ext!=NULL && ( strcmp(ext,".glm")==0 || strcmp(ext,".xml")==0 ) )
					*ext = '\0';
			}

			/* delete old checkpoint file if not desired */
			if ( global_checkpoint_keepall==0 && strcmp(fn,"")!=0 )
				unlink(fn);

			/* create current checkpoint save filename */
			sprintf(fn,"%s.%d",global_checkpoint_file,global_checkpoint_seqnum++);
			fp = fopen(fn,"w");
			if ( fp==NULL )
				output_error("unable to open checkpoint file '%s' for writing");
			else
			{
				if ( !stream_out(fp,SF_ALL) )
					output_error("checkpoint failure (stream context is %s)",stream_context());
				fclose(fp);
				last_checkpoint = now;
			}
		}
	}

}

/***********************************************************************/
//sjin: implement new ss_do_object_sync for pthreads
static void ss_do_object_sync(int thread, void *item)
{
	struct sync_data *data = &thread_data->data[thread];
	OBJECT *obj = (OBJECT *) item;
	TIMESTAMP this_t;
	
	//printf("thread %d\t%d\t%s\n", thread, obj->rank, obj->name);
	//this_t = object_sync(obj, global_clock, passtype[pass]);

	/* check in and out-of-service dates */
	if (global_clock<obj->in_svc)
		this_t = obj->in_svc; /* yet to go in service */
	else if (global_clock<=obj->out_svc)
	{
		this_t = object_sync(obj, global_clock, passtype[pass]);
#ifdef _DEBUG
		/* sync dumpfile */
		if (global_sync_dumpfile[0]!='\0')
		{
			static FILE *fp = NULL;
			if (fp==NULL)
			{
				static int tried = 0;
				if (!tried)
				{
					fp = fopen(global_sync_dumpfile,"wt");
					if (fp==NULL)
						output_error("sync_dumpfile '%s' is not writeable", global_sync_dumpfile);
					else
						fprintf(fp,"timestamp,pass,iteration,thread,object,sync\n");	
					tried = 1;
				}
			}
			if (fp!=NULL)
			{
				static int64 lasttime = 0;
				static char lastdate[64]="";
				char syncdate[64]="";
				static char *passname;
				static int lastpass = -1;
				char objname[1024];
				if (lastpass!=passtype[pass])
				{
					lastpass=passtype[pass];
					switch(lastpass) {
					case PC_PRETOPDOWN: passname = "PRESYNC"; break;
					case PC_BOTTOMUP: passname = "SYNC"; break;
					case PC_POSTTOPDOWN: passname = "POSTSYNC"; break;
					default: passname = "UNKNOWN"; break;
					}
				}
				if (lasttime!=global_clock)
				{
					lasttime = global_clock;
					convert_from_timestamp(global_clock,lastdate,sizeof(lastdate));
				}
				convert_from_timestamp(this_t<0?-this_t:this_t,syncdate,sizeof(syncdate));
				if (obj->name==NULL) sprintf(objname,"%s:%d", obj->oclass->name, obj->id);
				else strcpy(objname,obj->name);
				fprintf(fp,"%s,%s,%d,%d,%s,%s\n",lastdate,passname,global_iteration_limit-iteration_counter,thread,objname,syncdate);
			}
		}
#endif
	}
	else 
		this_t = TS_NEVER; /* already out of service */

	/* check for "soft" event (events that are ignored when stopping) */
	if (this_t < -1)
		this_t = -this_t;
	else if (this_t != TS_NEVER)
		data->hard_event++;  /* this counts the number of hard events */

	/* check for stopped clock */
	if (this_t < global_clock) {
		char b[64];
		output_error("%s: object %s stopped its clock (exec)!", simtime(), object_name(obj, b, 63));
		/* TROUBLESHOOT
			This indicates that one of the objects in the simulator has encountered a
			state where it cannot calculate the time to the next state.  This usually
			is caused by a bug in the module that implements that object's class.
		 */
		data->status = FAILED;
	} else {
		/* check for iteration limit approach */
		if (iteration_counter == 2 && this_t == global_clock) {
			char b[64];
			output_verbose("%s: object %s iteration limit imminent",
								simtime(), object_name(obj, b, 63));
		}
		else if (iteration_counter == 1 && this_t == global_clock) {
			output_error("convergence iteration limit reached for object %s:%d", obj->oclass->name, obj->id);
			/* TROUBLESHOOT
				This indicates that the core's solver was unable to determine
				a steady state for all objects for any time horizon.  Identify
				the object that is causing the convergence problem and contact
				the developer of the module that implements that object's class.
			 */
		}

		/* manage minimum timestep */
		if (global_minimum_timestep>1 && this_t>global_clock && this_t<TS_NEVER)
			this_t = (((this_t-1)/global_minimum_timestep)+1)*global_minimum_timestep;

		/* if this event precedes next step, next step is now this event */
		if (data->step_to > this_t) {
			//LOCK(data);
			data->step_to = this_t;
			//UNLOCK(data);
		}
		//printf("data->step_to=%d, this_t=%d\n", data->step_to, this_t);
	}
}

//sjin: implement new ss_do_object_sync_list for pthreads
static void *ss_do_object_sync_list(void *threadarg)
{
	LISTITEM *ptr;
	int iPtr;

	struct arg_data *mydata = (struct arg_data *) threadarg;
	int thread = mydata->thread;
	void *item = mydata->item;
	int incr = mydata->incr;

	iPtr = 0;
	for (ptr = item; ptr != NULL; ptr=ptr->next) {
		if (iPtr < incr) {
			ss_do_object_sync(thread, ptr->data);
			iPtr++;
		}
	}
	return NULL;
}

static STATUS init_all(void)
{
	OBJECT *obj;
	STATUS rv = SUCCESS;
	output_verbose("initializing objects...");

	/* initialize loadshapes */
	if (loadshape_initall()==FAILED || enduse_initall()==FAILED)
		return FAILED;

	TRY {
		for (obj=object_get_first(); obj!=NULL; obj=object_get_next(obj))
		{
			if (object_init(obj)==FAILED){
				char *b = (char *)malloc(64);
				memset(b, 0, 64);
				THROW("init_all(): object %s initialization failed", object_name(obj, b, 63));
				/* TROUBLESHOOT
					The initialization of the named object has failed.  Make sure that the object's
					requirements for initialization are satisfied and try again.
				 */
			}
			if((obj->oclass->passconfig & PC_FORCE_NAME) == PC_FORCE_NAME){
				if(0 == strcmp(obj->name, "")){
					output_warning("init: object %s:%d should have a name, but doesn't", obj->oclass->name, obj->id);
					/* TROUBLESHOOT
					   The object indicated has been flagged by the module which implements its class as one which must be named
					   to work properly.  Please provide the object with a name and try again.
					 */
				}
			}
		}
	} CATCH (char *msg) {
		output_error("init failure: %s", msg);
		/* TROUBLESHOOT
			The initialization procedure failed.  This is usually preceded 
			by a more detailed message that explains why it failed.  Follow
			the guidance for that message and try again.
		 */
		rv = FAILED;
	} ENDCATCH;
	return rv;
}

static TIMESTAMP commit_all(TIMESTAMP t0, TIMESTAMP t2){
	OBJECT *obj;
	TIMESTAMP min = TS_NEVER, curr = TS_NEVER;
	TRY {
		for (obj=object_get_first(); obj!=NULL; obj=object_get_next(obj))
		{
			if(obj->in_svc <= t0 && obj->out_svc >= t0){
				curr = object_commit(obj, t0, t2);
				if(curr == FAILED){
					char *b = (char *)malloc(64);
					memset(b, 0, 64);
					THROW("commit_all(): object %s commit failed", object_name(obj, b, 63));
					/* TROUBLESHOOT
						The commit function of the named object has failed.  Make sure that the object's
						requirements for commit'ing are satisfied and try again.  (likely internal state aberations)
					 */
				} else if(curr < min){
					min = curr;
				}
			}
		}
	} CATCH(char *msg){
		output_error("commit() failure: %s", msg);
		/* TROUBLESHOOT
			The commit'ing procedure failed.  This is usually preceded 
			by a more detailed message that explains why it failed.  Follow
			the guidance for that message and try again.
		 */
		min = TS_INVALID;
	} ENDCATCH;
	return min;
}

STATUS exec_test(struct sync_data *data, int pass, OBJECT *obj);
 
STATUS t_setup_ranks(void){
	return setup_ranks();
}
STATUS t_sync_all(PASSCONFIG pass)
{
	struct sync_data sync = {TS_NEVER,0,SUCCESS};
	TIMESTAMP start_time = global_clock;
	int pass_index = ((int)(pass/2)); /* 1->0, 2->1, 4->2; NB: if a fourth pass is added this won't work right */

	/* scan the ranks of objects */
	if (ranks[pass_index] != NULL)
	{
		int i;

		/* process object in order of rank using index */
		for (i = PASSINIT(pass_index); PASSCMP(i, pass_index); i += PASSINC(pass_index))
		{
			LISTITEM *item;
			/* skip empty lists */
			if (ranks[pass_index]->ordinal[i] == NULL) 
				continue;
			
			
			for (item=ranks[pass_index]->ordinal[i]->first; item!=NULL; item=item->next)
			{
				OBJECT *obj = item->data;
				if (exec_test(&sync,pass,obj)==FAILED)
					return FAILED;
			}
		}
	}

	/* run all non-schedule transforms */
	{
		TIMESTAMP st = scheduletransform_syncall(global_clock,XS_DOUBLE|XS_COMPLEX|XS_ENDUSE);// if (abs(t)<t2) t2=t;
		if (st<sync.step_to)
			sync.step_to = st;
	}

	return SUCCESS;
}

/* this function synchronizes all internal behaviors */
TIMESTAMP syncall_internals(TIMESTAMP t1)
{
	TIMESTAMP sc, ls, st, eu, t2;
	sc = schedule_syncall(t1);
	ls = loadshape_syncall(t1);// if (abs(t)<t2) t2=t;
	st = scheduletransform_syncall(t1,XS_SCHEDULE|XS_LOADSHAPE);// if (abs(t)<t2) t2=t;

	eu = enduse_syncall(t1);// if (abs(t)<t2) t2=t;
	/* @todo add other internal syncs here */
	t2 = TS_NEVER;
	if(sc < t2) t2 = sc;
	if(ls < t2) t2 = ls;
	if(st < t2) t2 = st;
	if(eu < t2) t2 = eu;
	return t2;
}

void exec_sleep(unsigned int usec)
{
#ifdef WIN32
	Sleep(usec/1000);
#else
	usleep(usec);
#endif
}

typedef struct s_objsyncdata {
	unsigned int n; // thread id 0~n_threads for this object rank list
	pthread_t pt;
	bool ok;
	//void *item;
	LISTITEM *ls;
	unsigned int nObj; // number of obj in this object rank list
	unsigned int t0;
	int i; // index of mutex or cond this object rank list uses 
} OBJSYNCDATA;

static pthread_mutex_t *startlock;
static pthread_mutex_t *donelock;
static pthread_cond_t *start;
static pthread_cond_t *done;

static unsigned int *next_t1;
static unsigned int *donecount;
static unsigned int *n_threads; //number of thread used in the threadpool of an object rank list

static void *obj_syncproc(void *ptr)
{
	OBJSYNCDATA *data = (OBJSYNCDATA*)ptr;
	LISTITEM *s;
	unsigned int n;
	int i = data->i;

	/*OBJECT *obj = data->ls->data;
	printf("%d %s %d\n", obj->id, obj->name, obj->rank);
	for (s=data->ls, n=0; s!=NULL, n<data->nObj; s=s->next,n++) {
		OBJECT *obj = s->data;
		//printf("thread %d, obj: %d %s %d\n", data->n, obj->id, obj->name, obj->rank);
		ss_do_object_sync(data->n, s->data);
	}*/
	
	// begin processing loop
	while (data->ok)
	{
		// lock access to start condition
		pthread_mutex_lock(&startlock[i]);
		// wait for thread start condition)
		///printf("old data->t0= %d ,next_t1[%d]= %d \n", data->t0, i, next_t1[i]);
		while (data->t0 == next_t1[i]) 
			pthread_cond_wait(&start[i], &startlock[i]);
		// unlock access to start count
		pthread_mutex_unlock(&startlock[i]);

		// process the list for this thread
		for (s=data->ls, n=0; s!=NULL, n<data->nObj; s=s->next,n++) {
			OBJECT *obj = s->data;
			//printf("thread %d, obj: %d %s %d\n", data->n, obj->id, obj->name, obj->rank);
			ss_do_object_sync(data->n, s->data);
		}

		// signal completed condition
		data->t0 = next_t1[i];
		///printf("new data->t0= %d ,next_t1[%d]= %d \n", data->t0, i, next_t1[i]);

		// lock access to done condition
		pthread_mutex_lock(&donelock[i]);
		// signal thread is done for now
		donecount[i] --; 
		///printf("donecount[%d]-- = %d from thread %d \n", i, donecount[i], data->n);
		// signal change in done condition
		pthread_cond_broadcast(&done[i]);
		//unlock access to done count
		pthread_mutex_unlock(&donelock[i]);
	}

	/*LISTITEM *ptr;
	int iPtr;

	struct arg_data *mydata = (struct arg_data *) threadarg;
	int thread = mydata->thread;
	void *item = mydata->item;
	int incr = mydata->incr;

	iPtr = 0;
	for (ptr = item; ptr != NULL; ptr=ptr->next) {
		if (iPtr < incr) {
			ss_do_object_sync(thread, ptr->data);
			iPtr++;
		}
	}
	return NULL;*/

	pthread_exit((void*)0);
	return (void*)0;
}

/** This is the main simulation loop
	@return STATUS is SUCCESS if the simulation reached equilibrium, 
	and FAILED if a problem was encountered.
 **/
STATUS exec_start(void)
{
	//sjin: remove threadpool
	//threadpool_t threadpool = INVALID_THREADPOOL;
	struct sync_data sync = {TS_NEVER,0,SUCCESS};
	TIMESTAMP start_time = global_clock;
	int64 passes = 0, tsteps = 0;
	int ptc_rv = 0;
	int ptj_rv = 0;
	time_t started_at = realtime_now();
	int j, k;

	//sjin: implement pthreads
	//pthread_t *thread_id;
	//sjin: add some variables for pthread implementation
	LISTITEM *ptr;
	int iPtr, incr;
	struct arg_data *arg_data_array;

	// Only setup threadpool for each object rank list at the first iteration;
	// After the first iteration, setTP = false;
	bool setTP = true; 
	//int n_threads; //number of thread used in the threadpool of an object rank list
	OBJSYNCDATA *thread = NULL;

	OBJECT *obj;

	int nObjRankList, iObjRankList;

	/* check for a model */
	if (object_get_count()==0)

		/* no object -> nothing to do */
		return SUCCESS;

	/* perform object initialization */
	if (init_all() == FAILED)
	{
		output_error("model initialization failed");
		/* TROUBLESHOOT
			The initialization procedure failed.  This is usually preceded 
			by a more detailed message that explains why it failed.  Follow
			the guidance for that message and try again.
		 */
		return FAILED;
	}

	/* establish rank index if necessary */
	if (ranks == NULL && setup_ranks() == FAILED)
	{
		output_error("ranks setup failed");
		/* TROUBLESHOOT
			The rank setup procedure failed.  This is usually preceded 
			by a more detailed message that explains why it failed.  Follow
			the guidance for that message and try again.
		 */
		return FAILED;
	}

	//sjin: print out obj information
	//for (obj=object_get_first(); obj!=NULL; obj=object_get_next(obj))
	//	printf("obj id: %d, rank = %d\n", obj->id, obj->rank);

	/* run checks */
	if (global_runchecks)
		return module_checkall();

	/* compile only check */
	if (global_compileonly)
		return SUCCESS;

	/* enable non-determinism check, if any */
	if (global_randomseed!=0 && global_threadcount>1)
		global_nondeterminism_warning = 1;

	if (!global_debug_mode)
	{
		/* schedule progress report event */
		if(global_show_progress){
			realtime_schedule_event(realtime_now()+1,show_progress);
		}

		/* set thread count equal to processor count if not passed on command-line */
		if (global_threadcount == 0)
			global_threadcount = processor_count();
		output_verbose("detected %d processor(s)", processor_count());
		output_verbose("using %d helper thread(s)", global_threadcount);

		//sjin: allocate arg_data_array to store pthreads creation argument
		arg_data_array = (struct arg_data *) malloc(sizeof(struct arg_data) 
						 * global_threadcount);

		/* allocate thread synchronization data */
		thread_data = (struct thread_data *) malloc(sizeof(struct thread_data) +
					  sizeof(struct sync_data) * global_threadcount);
		if (!thread_data) {
			output_error("thread memory allocation failed");
			/* TROUBLESHOOT
				A thread memory allocation failed.  
				Follow the standard process for freeing up memory ang try again.
			 */
			return FAILED;
		}
		thread_data->count = global_threadcount;
		thread_data->data = (struct sync_data *) (thread_data + 1);
		for (j = 0; j < thread_data->count; j++) 
			thread_data->data[j].status = SUCCESS;
	}
	else
	{
		output_debug("debug mode running single threaded");
		output_message("GridLAB-D entering debug mode");
	}

	/* realtime startup */
	if (global_run_realtime>0)
	{
		char buffer[64];
		time(&global_clock);
		output_verbose("realtime mode requires using now (%s) as starttime", convert_from_timestamp(global_clock,buffer,sizeof(buffer))>0?buffer:"invalid time");
		if (global_stoptime<global_clock)
			global_stoptime=TS_NEVER;
	}

	iteration_counter = global_iteration_limit;
	sync.step_to = global_clock;
	sync.hard_event = 1;

	/* signal handler */
	signal(SIGABRT, exec_sighandler);
	signal(SIGINT, exec_sighandler);
	signal(SIGTERM, exec_sighandler);

	// count how many object rank list in one iteration
	nObjRankList = 0;
	/* scan the ranks of objects */
	for (pass = 0; ranks[pass] != NULL; pass++)
	{
		int i;
		/* process object in order of rank using index */
		for (i = PASSINIT(pass); PASSCMP(i, pass); i += PASSINC(pass))
		{
			/* skip empty lists */
			if (ranks[pass]->ordinal[i] == NULL) 
				continue;
			nObjRankList++; // count how many object rank list in one iteration
		}
	}

	/* allocate and initialize thread data */
	output_debug("nObjRankList=%d ",nObjRankList);

	next_t1 = malloc(sizeof(next_t1[0])*nObjRankList);
	memset(next_t1,0,sizeof(next_t1[0])*nObjRankList);

	donecount = malloc(sizeof(donecount[0])*nObjRankList);
	memset(donecount,0,sizeof(donecount[0])*nObjRankList);

	n_threads = malloc(sizeof(n_threads[0])*nObjRankList);
	memset(n_threads,0,sizeof(n_threads[0])*nObjRankList);

	// allocation and nitialize mutex and cond for object rank lists
	startlock = malloc(sizeof(startlock[0])*nObjRankList);
	donelock = malloc(sizeof(donelock[0])*nObjRankList);
	start = malloc(sizeof(start[0])*nObjRankList);
	done = malloc(sizeof(done[0])*nObjRankList);
	for(k=0;k<nObjRankList;k++) 
	{
		pthread_mutex_init(&startlock[k], NULL);
		pthread_mutex_init(&donelock[k], NULL);
		pthread_cond_init(&start[k], NULL);
		pthread_cond_init(&done[k], NULL);
	}

	//sjin: GetMachineCycleCount
	//mc_start_time = GetMachineCycleCount();
	cstart = clock();
	//thread_id = (pthread_t *) malloc(global_threadcount * sizeof(pthread_t));
	/* main loop exception handler */
	TRY {

		/* main loop runs for iteration limit, or when nothing futher occurs (ignoring soft events) */
		int running; /* split into two tests to make it easier to tell what's going on */
		while ( running = (sync.step_to <= global_stoptime && sync.step_to < TS_NEVER && sync.hard_event>0),
			iteration_counter>0 && ( running || global_run_realtime>0) && !stop_now ) 
		{
			///printf("\n!!!!!!!!!!!!!!!!!!!!!New iteration started:!!!!!!!!!!!!!!!!!!!!!!!\n\n");
		
			do_checkpoint();

			//printf("Iteration increased!\n\n");
			/* set time context */
			output_set_time_context(sync.step_to);

			sync.hard_event = (global_stoptime == TS_NEVER ? 0 : 1);

			/* realtime support */
			if (global_run_realtime>0)
			{
#ifdef WIN32
				struct timeb tv;
				ftime(&tv);
				output_verbose("waiting %d msec", 1000-tv.millitm);
				Sleep(1000-tv.millitm );
				if ( global_run_realtime==1 )
					global_clock = tv.time + global_run_realtime;
				else
					global_clock += global_run_realtime;
#else
				struct timeval tv;
				gettimeofday(&tv);
				output_verbose("waiting %d usec", 1000000-tv.tv_usec);
				usleep(1000000-tv.tv_usec);
				if ( global_run_realtime==1 )
					global_clock = tv.tv_sec+global_run_realtime;
				else
					global_clock += global_run_realtime;
#endif
				output_verbose("realtime clock advancing to %d", (int)global_clock);
			}
			else
				global_clock = sync.step_to;

			/* synchronize all internal schedules */
			///printf("global_clock=%d\n",global_clock);
			sync.step_to = syncall_internals(global_clock);
			if(sync.step_to!=TS_NEVER && sync.step_to <= global_clock){
				THROW("internal property sync failure");
				/* TROUBLESHOOT
					An internal property such as schedule, enduse or loadshape has failed to synchronize and the simulation aborted.
					This message should be preceded by a more informative message that explains which element failed and why.
					Follow the troubleshooting recommendations for that message and try again.
				 */
			}

			if (!global_debug_mode)
			{
				for (j = 0; j < thread_data->count; j++) {
					thread_data->data[j].hard_event = 0;
					thread_data->data[j].step_to = TS_NEVER;
				}
			}

			iObjRankList = -1;
			/* scan the ranks of objects */
			for (pass = 0; ranks[pass] != NULL; pass++)
			{
				int i;

				///printf("\npass %d::::::::::::::::::::::::::::::::::::\n",pass);

				/* process object in order of rank using index */
				for (i = PASSINIT(pass); PASSCMP(i, pass); i += PASSINC(pass))
				{
					/* skip empty lists */
					if (ranks[pass]->ordinal[i] == NULL) 
						continue;

					iObjRankList ++;
					///printf("\nProcessing object rank list %d:..................\n", iObjRankList);

					if (global_debug_mode)
					{
						LISTITEM *item;
						for (item=ranks[pass]->ordinal[i]->first; item!=NULL; item=item->next)
						{
							OBJECT *obj = item->data;
							if (exec_debug(&sync,pass,i,obj)==FAILED)
								THROW("debugger quit");
						}
					}
					else
					{
						//sjin: if global_threadcount == 1, no pthread multhreading
						if (global_threadcount == 1) {
							for (ptr = ranks[pass]->ordinal[i]->first; ptr != NULL; ptr=ptr->next) {
								OBJECT *obj = ptr->data;
								ss_do_object_sync(0, ptr->data);					
								///printf("%d %s %d\n", obj->id, obj->name, obj->rank);
							}
							//printf("\n");
						} else { //sjin: implement pthreads
							int n_items,objn=0,n;
							int n_obj = ranks[pass]->ordinal[i]->size;
							/*printf("ranks[%d]->ordinal[%d]->size=%d\n",pass,i,n_obj);
							for (ptr = ranks[pass]->ordinal[i]->first; ptr != NULL; ptr=ptr->next) {
								OBJECT *obj = ptr->data;
								printf("%d %s %d\n", obj->id, obj->name, obj->rank);
							}*/
							// Only create threadpool for each object rank list at the first iteration. 
							// Reuse the threadppol of each object rank list at all other iterations.
							if (setTP == true) { 
								incr = (int)ceil((float) n_obj / global_threadcount);
								// if the number of objects is less than or equal to the number of threads, each thread process one object 
								if (incr <= 1) {
									n_threads[iObjRankList] = n_obj;
									n_items = 1;
								// if the number of objects is greater than the number of threads, each thread process the same number of 
								// objects (incr), except that the last thread may process less objects 
								} else {
									n_threads[iObjRankList] = (int)ceil((float) n_obj / incr);
									n_items = incr;
								}
								if (n_threads[iObjRankList] > global_threadcount) {
									output_error("Running threads > global_threadcount");
									exit(0);
								}
								///printf("incr=%d,n_threads=%d,n_items=%d\n",incr,n_threads[iObjRankList],n_items);
								///printf("\nn_threads[%d]=%d, n_items=%d\n",iObjRankList,n_threads[iObjRankList],n_items);
								// allocate thread list
								thread = (OBJSYNCDATA*)malloc(sizeof(OBJSYNCDATA)*n_threads[iObjRankList]);
								memset(thread,0,sizeof(OBJSYNCDATA)*n_threads[iObjRankList]);
								// assign starting obj for each thread
								for (ptr=ranks[pass]->ordinal[i]->first;ptr!=NULL;ptr=ptr->next)
								{
									if (thread[objn].nObj==n_items)
										objn++;
									if (thread[objn].nObj==0) {
										thread[objn].ls=ptr;
									}
									thread[objn].nObj++;
								}
								// create threads
								for (n=0; n<n_threads[iObjRankList]; n++) {
									//printf("thread %d: *********************\n",n);
									thread[n].ok = true;
									thread[n].i = iObjRankList;
									/*for (ptr = thread[n].ls, k=0; ptr != NULL, k<thread[n].nObj; ptr=ptr->next, k++) {
										OBJECT *obj = ptr->data;
										printf("%d %s %d\n", obj->id, obj->name, obj->rank);
									}*/
									/*obj = thread[n].ls->data;
									printf("%d %s %d\n", obj->id, obj->name, obj->rank);*/
									if (pthread_create(&(thread[n].pt),NULL,obj_syncproc,&(thread[n]))!=0) {
										output_fatal("obj_sync thread creation failed");
										thread[n].ok = false;
									} else
										thread[n].n = n;
								}

							}
														
							// lock access to done count
							pthread_mutex_lock(&donelock[iObjRankList]);
							
							// initialize wait count
							donecount[iObjRankList] = n_threads[iObjRankList];
							///printf("donecount[%d]=%d\n",iObjRankList,donecount[iObjRankList]);

							// lock access to start condition
							pthread_mutex_lock(&startlock[iObjRankList]);
							// update start condition
							next_t1[iObjRankList] ++;
							///printf("next_t1[%d]=%d\n",iObjRankList,next_t1[iObjRankList]);
							// signal all the threads
							pthread_cond_broadcast(&start[iObjRankList]);
							// unlock access to start count
							pthread_mutex_unlock(&startlock[iObjRankList]);

							// begin wait
							while (donecount[iObjRankList]>0)
								pthread_cond_wait(&done[iObjRankList],&donelock[iObjRankList]);
							// unlock done count
							pthread_mutex_unlock(&donelock[iObjRankList]);
						
							///printf("Done!\n");
						}

						//Sleep(5);		

						for (j = 0; j < thread_data->count; j++) {
							if (thread_data->data[j].status == FAILED) {
								sync.status = FAILED;
								THROW("synchonization failed");
							}
						}
					}
				}


				/* run all non-schedule transforms */
				{
					TIMESTAMP st = scheduletransform_syncall(global_clock,XS_DOUBLE|XS_COMPLEX|XS_ENDUSE);// if (abs(t)<t2) t2=t;
					if (st<sync.step_to)
						sync.step_to = st;
				}
			}
			setTP = false;

			if (!global_debug_mode)
			{
				for (j = 0; j < thread_data->count; j++) {
					sync.hard_event += thread_data->data[j].hard_event;
					if (thread_data->data[j].step_to < sync.step_to)
						sync.step_to = thread_data->data[j].step_to;
				}

				/* report progress */
				realtime_run_schedule();
			}

			/* count number of passes */
			passes++;

			/* check for clock advance */
			if (sync.step_to != global_clock)
			{
				TIMESTAMP commit_time = TS_NEVER;
				commit_time = commit_all(global_clock, sync.step_to);
				if (commit_time <= global_clock)
				{
					// commit cannot force reiterations, and any event where the time is less than the global clock
					//  indicates that the object is reporting a failure
					output_error("model commit failed");
					/* TROUBLESHOOT
						The commit procedure failed.  This is usually preceded 
						by a more detailed message that explains why it failed.  Follow
						the guidance for that message and try again.
					 */
					return FAILED;
				} else if(commit_time < sync.step_to){
					sync.step_to = commit_time;
				}
				/* reset iteration count */
				iteration_counter = global_iteration_limit;

				/* count number of timesteps */
				tsteps++;
			}
			/* check iteration limit */
			else if (--iteration_counter == 0)
			{
				output_error("convergence iteration limit reached at %s (exec)", simtime());
				/* TROUBLESHOOT
					This indicates that the core's solver was unable to determine
					a steady state for all objects for any time horizon.  Identify
					the object that is causing the convergence problem and contact
					the developer of the module that implements that object's class.
				 */
				sync.status = FAILED;
				THROW("convergence failure");
			}
		} // end of while loop

		/* disable signal handler */
		signal(SIGINT,NULL);

		/* check end state */
		if (sync.step_to==TS_NEVER)
		{
			char buffer[64];
			output_verbose("simulation at steady state at %s", convert_from_timestamp(global_clock,buffer,sizeof(buffer))?buffer:"invalid time");
		}
	}
	CATCH(char *msg)
	{
		output_error("exec halted: %s", msg);
		sync.status = FAILED;
		/* TROUBLESHOOT
			This indicates that the core's solver shut down.  This message
			is usually preceded by more detailed messages.  Follow the guidance
			for those messages and try again.
		 */
	}
	ENDCATCH

	//sjin: GetMachineCycleCount
	//mc_end_time = GetMachineCycleCount();
	//printf("%ld\n",(mc_end_time-mc_start_time));
	cend = clock();
	//printf("%f\n", (double)(cend - cstart) / (double)CLOCKS_PER_SEC);

	/* deallocate threadpool */
	if (!global_debug_mode)
	{
		//sjin: remove threadpool
		//tp_release(threadpool);
		free(thread_data);

#ifdef NEVER
		/* wipe out progress report */
		if (!global_keep_progress)
			output_raw("                                                           \r"); 
#endif
	}

	// Destroy mutex and cond
	for(k=0;k<nObjRankList;k++) {
		pthread_mutex_destroy(&startlock[k]);
		pthread_mutex_destroy(&donelock[k]);
		pthread_cond_destroy(&start[k]);
		pthread_cond_destroy(&done[k]);
	}

	/* report performance */
	if (global_profiler && sync.status==SUCCESS)
	{
		double elapsed_sim = (timestamp_to_hours(global_clock<start_time?start_time:global_clock)-timestamp_to_hours(start_time));
		double elapsed_wall = (double)(realtime_now()-started_at+1);
		double sync_time = 0;
		double sim_speed = object_get_count()/1000.0*elapsed_sim/elapsed_wall;

		extern clock_t loader_time;
		extern clock_t schedule_synctime;
		extern clock_t loadshape_synctime;
		extern clock_t enduse_synctime;
		extern clock_t transform_synctime;

		CLASS *cl;
		if (global_threadcount==0) global_threadcount=1;
		for (cl=class_get_first_class(); cl!=NULL; cl=cl->next)
			sync_time += ((double)cl->profiler.clocks)/CLOCKS_PER_SEC;
		sync_time /= global_threadcount;

		output_profile("\nCore profiler results");
		output_profile("======================\n");
		output_profile("Total objects           %8d objects", object_get_count());
		output_profile("Parallelism             %8d thread%s", global_threadcount,global_threadcount>1?"s":"");
		output_profile("Total time              %8.1f seconds", elapsed_wall);
		output_profile("  Core time             %8.1f seconds (%.1f%%)", (elapsed_wall-sync_time),(elapsed_wall-sync_time)/elapsed_wall*100);
		output_profile("    Compiler            %8.1f seconds (%.1f%%)", (double)loader_time/CLOCKS_PER_SEC,((double)loader_time/CLOCKS_PER_SEC)/elapsed_wall*100);
		output_profile("    Schedules           %8.1f seconds (%.1f%%)", (double)schedule_synctime/CLOCKS_PER_SEC,((double)schedule_synctime/CLOCKS_PER_SEC)/elapsed_wall*100);
		output_profile("    Loadshapes          %8.1f seconds (%.1f%%)", (double)loadshape_synctime/CLOCKS_PER_SEC,((double)loadshape_synctime/CLOCKS_PER_SEC)/elapsed_wall*100);
		output_profile("    Enduses             %8.1f seconds (%.1f%%)", (double)enduse_synctime/CLOCKS_PER_SEC,((double)enduse_synctime/CLOCKS_PER_SEC)/elapsed_wall*100);
		output_profile("    Transforms          %8.1f seconds (%.1f%%)", (double)transform_synctime/CLOCKS_PER_SEC,((double)transform_synctime/CLOCKS_PER_SEC)/elapsed_wall*100);
		output_profile("  Model time            %8.1f seconds/thread (%.1f%%)", sync_time,sync_time/elapsed_wall*100);
		output_profile("Simulation time         %8.0f days", elapsed_sim/24);
		if (sim_speed>10.0)
			output_profile("Simulation speed        %7.0lfk object.hours/second", sim_speed);
		else if (sim_speed>1.0)
			output_profile("Simulation speed        %7.1lfk object.hours/second", sim_speed);
		else
			output_profile("Simulation speed        %7.0lf object.hours/second", sim_speed*1000);
		output_profile("Syncs completed         %8d passes", passes);
		output_profile("Time steps completed    %8d timesteps", tsteps);
		output_profile("Convergence efficiency  %8.02lf passes/timestep", (double)passes/tsteps);
#ifndef NOLOCKS
		output_profile("Memory lock contention  %7.01lf%%", (lock_spin>0 ? (1-(double)lock_count/(double)lock_spin)*100 : 0));
#endif
		output_profile("Average timestep        %7.0lf seconds/timestep", (double)(global_clock<start_time?0:global_clock-start_time)/tsteps);
		output_profile("Simulation rate         %7.0lf x realtime", (double)(global_clock<start_time?0:global_clock-start_time)/elapsed_wall);
		output_profile("\n");
	}

	return sync.status;
}

/** Starts the executive test loop 
	@return STATUS is SUCCESS if all test passed, FAILED is any test failed.
 **/
STATUS exec_test(struct sync_data *data, /**< the synchronization state data */
				 int pass, /**< the pass number */
				 OBJECT *obj){ /**< the current object */
	TIMESTAMP this_t;
	/* check in and out-of-service dates */
	if (global_clock<obj->in_svc)
		this_t = obj->in_svc; /* yet to go in service */
	else if (global_clock<=obj->out_svc)
		this_t = object_sync(obj, global_clock, pass);
	else 
		this_t = TS_NEVER; /* already out of service */

	/* check for "soft" event (events that are ignored when stopping) */
	if (this_t < -1)
		this_t = -this_t;
	else if (this_t != TS_NEVER)
		data->hard_event++;  /* this counts the number of hard events */

	/* check for stopped clock */
	if (this_t < global_clock) {
		char b[64];
		output_error("%s: object %s stopped its clock! (test)", simtime(), object_name(obj, b, 63));
		/* TROUBLESHOOT
			This indicates that one of the objects in the simulator has encountered a
			state where it cannot calculate the time to the next state.  This usually
			is caused by a bug in the module that implements that object's class.
		 */
		data->status = FAILED;
	} else {
		/* check for iteration limit approach */
		if (iteration_counter == 2 && this_t == global_clock) {
			char b[64];
			output_verbose("%s: object %s iteration limit imminent",
								simtime(), object_name(obj, b, 63));
		}
		else if (iteration_counter == 1 && this_t == global_clock) {
			output_error("convergence iteration limit reached for object %s:%d (test)", obj->oclass->name, obj->id);
			/* TROUBLESHOOT
				This indicates that one of the objects in the simulator has encountered a
				state where it cannot calculate the time to the next state.  This usually
				is caused by a bug in the module that implements that object's class.
			 */
		}

		/* manage minimum timestep */
		if (global_minimum_timestep>1 && this_t>global_clock && this_t<TS_NEVER)
			this_t = ((this_t/global_minimum_timestep)+1)*global_minimum_timestep;

		/* if this event precedes next step, next step is now this event */
		if (data->step_to > this_t)
			data->step_to = this_t;
		data->status = SUCCESS;
	}
	return data->status;
}

/**@}*/
