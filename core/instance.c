/* @file instance.c
   Copyright (C) 2011, Battelle Memorial Institute
 */

#include <math.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef WIN32
#define _WIN32_WINNT 0x0400
#include <windows.h>
#endif

#include <pthread.h>

#include "instance.h"
#include "output.h"
#include "globals.h"
#include "random.h"
#include "exec.h"

clock_t instance_synctime = 0;

#define MSGALLOCSZ 1024

/** message_init
    Initialize a message cache for communication between master and slave.
	This function zeros the buffer and sets the usage to the first data
	entry (all but the header is unused).
	@returns Nothing
 **/
static void message_init(MESSAGE *msg, /**< pointer to the message cache */
						 size_t size)  /**< size of the message cache */
{
	msg->usize = sizeof(MESSAGE)-1;
	msg->asize = size;
	msg->ts = TS_ZERO;
	memset(&(msg->data),0,size);
}

/** message_new
    Creates a new message cache for communications between master and slave.
	This function allocated memory for a new message cache.
	@returns a pointer to the next message cache.
	@todo This function must be rewritten because the new cache must be placed
	in the shared memory map and cannot be taken from heap.
 **/
static MESSAGE *message_new(size_t size) /** size of the new message cache */
{
	MESSAGE *msg;
	if (size==0) size = sizeof(MESSAGE)+MSGALLOCSZ;
	msg = malloc(size);
	if ( !msg ) return NULL;

	/* initialize message */
	message_init(msg,size);
	output_fatal("message cache be created using existing code");
	return msg;
}

/** message_grow
    Increases the size of the message cache for communications between master and slave.
	This function create a new message cache, copy the old one and destroys the old one.
	@return a pointer to the new message cache
	@see message_new
	@todo This function must be rewritten because the new cache must be placed
	in the shared memory map and cannot be taken from heap.
 **/
static MESSAGE *message_grow(MESSAGE *old) /**< pointer to the message cache */
{
	size_t size = old->asize+MSGALLOCSZ;
	MESSAGE *msg = message_new(size);
	free(old);
	output_fatal("message cache cannot grow using existing code");
	return msg;
}

/** message_add
    Add a buffer to the message buffer
	@returns address of the message buffer, or -1 for failure
	@note If the cache becomes too small it will grow.
	@see message_grow
 **/
char *message_add(MESSAGE **msg,	/**< pointer to message addr to which buffer is added (may be changed) */
				   size_t size)	/**< size of buffer to be added */
{
	size_t pos, need;

	/* create a new message if needed */
	if ( (*msg)==NULL )
		*msg = message_new(0);

	/* if message creation failed */
	if ( (*msg)==NULL )
		return NULL;

	/* determine position of need data */
	pos = (*msg)->usize;

	/* determine new size of message */
	need = pos+size;

	/* if size exceed allocation */
	if ( need>(*msg)->asize )

		/* grow the message buffer */
		(*msg) = message_grow(*msg);

	/* if grow failed */
	if ( (*msg)==NULL )
		return NULL;

	/* update the buffer usage */
	(*msg)->usize = need;

	/* return the position of the new data */
	return ((char*)(*msg))+pos;
}


/////////////////////////////////////////////////////////////////////
// MASTER INSTANCE

static instance *instance_list = NULL;
int instances_count = 0;
int instances_exited = 0;

/** instance_runproc
    This function is the slave instance creation and monitoring thread.
	If the instance host is the local machine, a new instance of gridlabd is started
	with the connection string as the first command argument. The global values of
	verbose and debug are consulted and the appropriate command line options are 
	added if needed to ensure the slave has the same mode.
	@todo If the instance host is a remote machine, an RPC call is made to start the
	new instance of gridlabd on the remote machine with the connection string as the
	first command argument.
	@returns Slave exit code when the slave exits
 **/
void *instance_runproc(void *ptr)
{
	instance *inst = (instance*)ptr;
	char cmd[1024];
	int64 rc;

	/* run new instance */
	if ( strcmp(inst->hostname,"localhost")==0)

		/* local instance of gridlabd */
		sprintf(cmd,"%s/gridlabd %s %s --slave %s:%"FMT_INT64"x %s &", global_execdir, global_verbose_mode?"--verbose":"", global_debug_output?"--debug":"", global_hostname,inst->cacheid, inst->model);
	else
		/* @todo RPC call to remote machine instance of gridlabd */
		sprintf(cmd,"%s/gridlabd %s %s --slave %s:%d %s &", global_execdir, global_verbose_mode?"--verbose":"", global_debug_mode?"--debug":"", global_hostname,global_server_portnum, inst->model);

	output_debug("starting new instance with command '%s'", cmd);
	rc = system(cmd);

	/* instance exited */
	output_verbose("instance for model '%s' terminated with exit code %d (%s)",inst->model, rc, rc<0?strerror(errno):"gridlabd error"); 
	instances_exited++;

	return (void*)rc;
}

/** instance_create
    This function creates a new slave instance object in the current instance's
	list of slaves.
	@returns a pointer to the new instance object.
 **/
instance *instance_create(char *host)
{
	instance *inst;
	
	/* allocate memory for new instance */
	inst = (instance*)malloc(sizeof(instance));
	if ( !inst )
	{
		output_error("unable to allocate memory for instance on %s", host);
		return NULL;
	}

	/* initialize instance */
	memset(inst,0,sizeof(instance));
	strncpy(inst->hostname,host,sizeof(inst->hostname)-1);
	inst->cacheid = random_id();

	/* add to instance list */
	inst->next = instance_list;
	instance_list = inst;

	output_verbose("defining new instance on %s", host);

	return inst;
}

/** instance_add_linkage
	This function add a linkage to an instance.
	@returns 1 on success, 0 on failure.
 **/
int instance_add_linkage(instance *inst, linkage *lnk)
{
	if ( lnk->type==LT_MASTERTOSLAVE )
	{
		lnk->next = inst->write;
		inst->write = lnk;
		return 1;
	}
	else if ( lnk->type==LT_SLAVETOMASTER )
	{
		lnk->next = inst->read;
		inst->read = lnk;
		return 1;
	}
	else
	{
		return 0;
	}
}

/** instance_master_wait
	Wait the master into a wait state for all the slave->master signal.
 **/
int instance_master_wait(void)
{
	instance *inst;
	int status = 0;
	for ( inst=instance_list ; inst!=NULL ; inst=inst->next )
	{
		output_debug("master waiting on slave %d", inst->id);
#ifdef WIN32
		{	DWORD rc = WaitForSingleObject(inst->hMaster,global_signal_timeout);
			switch ( rc ) {
			case WAIT_ABANDONED:
				output_error("slave %d wait abandoned", inst->id);
				break;
			case WAIT_OBJECT_0:
				output_debug("slave %d wait completed", inst->id);
				status = 1;
				break;
			case WAIT_TIMEOUT:
				output_error("slave %d wait timeout", inst->id);
				break;
			case WAIT_FAILED:
				output_error("slave %d wait failed (error code %d)",inst->id,GetLastError());
				break;
			default:
				output_error("slave %d wait return code not recognized (rc=%d)", inst->id,rc);
				break;
			}
		}
		ResetEvent(inst->hMaster);
#else
	// @todo linux/unix slave signalling
#endif
		output_debug("master resuming");
	}
	exec_mls_resume(TS_NEVER);
	return status;
}

/** instance_master_done
    Signal all the slaves that the master is done processing the next sync state.
 **/
void instance_master_done(TIMESTAMP t1)
{
	instance *inst;
	for ( inst=instance_list ; inst!=NULL ; inst=inst->next )
	{
		inst->cache->ts = t1;
		output_debug("master signaling slave %d with timestamp %"FMT_INT64"d", inst->id, inst->cache->ts);
#ifdef WIN32
		SetEvent(inst->hMaster);
#else
	// @todo linux/unix slave signalling
#endif
	}
}

/** instance_init
    Initialize an instance object.
	return 1 on failure, 0 on success.
 **/
int instance_init(instance *inst)
{
	linkage *lnk;

	/* check for looping model */
	if (strcmp(inst->model,global_modelname)==0)
	{
		output_error("slave instance model '%s' is the same as the master model '%'", inst->model,global_modelname);
		return 1;
	}

	inst->cachesize = sizeof(MESSAGE)+MSGALLOCSZ; /* @todo size instance cache dynamically from linkage list */
	{
#ifdef WIN32
		char cachename[1024];
		char eventname[64];
		SECURITY_ATTRIBUTES secAttr = {sizeof(SECURITY_ATTRIBUTES),(LPSECURITY_ATTRIBUTES)NULL,TRUE}; 

		/* setup cache */
		sprintf(cachename,"GLD-%"FMT_INT64"x",inst->cacheid);
		inst->hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,cachename);
		if ( !inst->hMap )
		{
			inst->hMap = CreateFileMapping(INVALID_HANDLE_VALUE,&secAttr,PAGE_READWRITE|SEC_COMMIT, 0,(DWORD)inst->cachesize, cachename);
			if ( !inst->hMap )
			{
				output_error("unable to create cache '%s' for instance '%s' (error code %d)", cachename, inst->model, GetLastError());
				/* TROUBLESHOOT
				   Error code 1314 indicates that the "SeCreateGlobalPrivilege" has not been granted by the system administrator.
				   This privilege is required for GridLAB-D to use shared memory in multirun mode.
				   */
				return 1;
			}
			else
				output_debug("cache '%s' created for instance '%s'", cachename, inst->model);
		}
		else
		{
			output_error("cache '%s' for instance '%s' already in use", cachename, inst->model);
			return 1;
		}
		inst->cache = (void*)MapViewOfFile(inst->hMap,FILE_MAP_ALL_ACCESS,0,0,(DWORD)inst->cachesize);
		if ( !inst->cache )
		{
			output_error("unable to map view of cache '%s' for instance '%s' already in use", cachename, inst->model);
			return 1;
		}
		else
			output_debug("cache '%s' map view for model '%s' ok", cachename, inst->model);

		/* initialize slave instance cache */
		message_init(inst->cache,inst->cachesize);
		inst->cache->id = inst->id = instances_count;
		output_verbose("slave %d assigned to '%s'", inst->id, inst->model);
		output_debug("slave %d cache size is %d of %d bytes allocated", inst->id, inst->cache->usize, inst->cache->asize);

		/* setup master signalling event */
		sprintf(eventname,"GLD-%"FMT_INT64"x-M", inst->cacheid);
		inst->hMaster = CreateEvent(&secAttr,FALSE,FALSE,eventname); /* initially unsignalled */
		if ( !(inst->hMaster) )
		{
			output_error("unable to create event signal '%s' for slave %d", eventname, inst->id);
			return 1;
		}
		else
			output_debug("created event signal '%s' for slave%d ", eventname, inst->id);

		/* setup master signalling event */
		sprintf(eventname,"GLD-%"FMT_INT64"x-S", inst->cacheid);
		inst->hSlave = CreateEvent(&secAttr,FALSE,FALSE,eventname); /* initially unsignalled */
		if ( !(inst->hSlave) )
		{
			output_error("unable to create event signal '%s' for slave %d", eventname, inst->id);
			return 1;
		}
		else
			output_debug("created event signal '%s' for slave %d", eventname, inst->id);
#else
	// @todo linux/unix master signalling
#endif
	}

	/* initialize linkages */
	for ( lnk=inst->write ; lnk!=NULL ; lnk=lnk->next )
		if ( linkage_init(inst, lnk)==FAILED )
			return 0;
	for ( lnk=inst->read ; lnk!=NULL ; lnk=lnk->next )
		if ( linkage_init(inst, lnk)==FAILED )
			return 0;
	output_debug("linkages for slave %d initialization complete", inst->id);
	
	/* start the slave instance */
	if ( pthread_create(&(inst->threadid), NULL, instance_runproc, (void*)inst) )
	{
		output_error("unable to starte instance slave %d", inst->id);
		return 1;
	}
	else
	{
		output_verbose("started instance for slave %d", inst->id);
		instances_count++;
		return 0;
	}
}

/** instance_initall
	Initialize all instance objects.
	@note This function waits for the slaves to signal their initialization has completed using instance_slave_done().
	@see instance_slave_done
	@returns SUCCESS or FAILED.
 **/
int instance_initall(void)
{
	instance *inst;
	if ( instance_list ) 
	{
		global_multirun_mode = MRM_MASTER;
		output_verbose("entering multirun mode");
		output_prefix_enable();
	}
	for ( inst=instance_list ; inst!=NULL ; inst=inst->next )
	{
		if ( instance_init(inst) )
			return FAILED;
	}

	// wait for slaves to signal init done
	instance_master_wait();
	return SUCCESS;
}

/** instance_write_slave
    Write linkages to slaves.
 **/
void instance_write_slave(instance *inst)
{
	linkage *lnk;

	/* write output to instance */
	for ( lnk=inst->write ; lnk!=NULL ; lnk=lnk->next )
		linkage_master_to_slave(lnk);
}

/** instance_read_slaves
    Read linkages from slaves (including next time)
 **/
TIMESTAMP instance_read_slave(instance *inst)
{
	TIMESTAMP t2 = TS_INVALID;
	linkage *lnk;

	/* wait for slave */
	output_debug("master waiting on slave %d", inst->id);
#ifdef WIN32
	{	DWORD rc = WaitForSingleObject(inst->hMaster,global_signal_timeout);
		switch ( rc ) {
		case WAIT_ABANDONED:
			output_error("slave %d wait abandoned", inst->id);
			break;
		case WAIT_OBJECT_0:
			output_debug("slave %d wait completed", inst->id);
			t2 = inst->cache->ts;
			break;
		case WAIT_TIMEOUT:
			output_error("slave %d wait timeout", inst->id);
			break;
		case WAIT_FAILED:
			output_error("slave %d wait failed (error code %d)",inst->id,GetLastError());
			break;
		default:
			output_error("slave %d wait return code not recognized (rc=%d)", inst->id,rc);
			break;
		}
	}
	ResetEvent(inst->hMaster);
#else
	// @todo linux/unix slave signalling
#endif
	output_debug("master resuming");

	/* @todo read input from instance */
	for ( lnk=inst->read ; lnk!=NULL ; lnk=lnk->next )
		linkage_slave_to_master(lnk);

	/* return the next time */
	return t2;
}

/** instance_syncall
    Synchronize all slave instances
	@return the next time, TS_NEVER if slave is done, and TS_INVALID is sync failed.

	@todo It would be better to give each slave its own thread
	so the readback doesn't have to wait until that last slave
	signals it's done. This would be much more like the method
	used to parallelize other sync loops only without the use
	of multiple lock events.
 **/
TIMESTAMP instance_syncall(TIMESTAMP t1)
{
	/* only process if instances exist */
	if ( instance_list )
	{
		TIMESTAMP t2 = TS_NEVER;
		clock_t ts = clock();
		instance *inst;

		/* check to see if an instance was lost */
		if ( instances_exited>0 )
		{
			output_debug("%d instance exit detected, stopping main loop", instances_exited);
	
			/* tell main too to stop */
			return TS_INVALID;
		}
	
		/* send linkage to slaves */
		for ( inst=instance_list ; inst!=NULL ; inst=inst->next )
			instance_write_slave(inst);
	
		/* signal slaves to start */
		instance_master_done(t1);
	
		/* read linkages from slaves */
		for ( inst=instance_list ; inst!=NULL ; inst=inst->next )
		{
			TIMESTAMP t3 = instance_read_slave(inst);
			if ( t3 < t2 ) t2 = t3;
		}
	
		output_debug("instance sync time is %"FMT_INT64"d", t2);
		instance_synctime += clock() - ts;
		return t2;
	}
	else
		return TS_NEVER;
}

/////////////////////////////////////////////////////////////////////
// SLAVE INSTANCE

static MESSAGE *slave_cache;
#ifdef WIN32
static HANDLE hMap;
static HANDLE slave_eventHandle;
static HANDLE master_eventHandle;
#else
	// @todo linux/unix slave signalling
#endif
unsigned int slave_id;
pthread_t slave_tid;
static struct {
	linkage *read, *write;
} master;

/** instance_slaveproc
    Create main slave control loop to maintain sync with master.
 **/
void *instance_slaveproc(void *ptr)
{
	linkage *lnk;
	output_debug("slave %d controller startup in progress", slave_id);
	while (slave_cache->ts!=TS_NEVER)
	{
		/* wait for master to signal slave */
		if ( !instance_slave_wait() )
		{
			/* stop the main loop and exit the slave controller */
			exec_mls_done();
			return NULL;
		}

		/* copy inbound linkages */
		for ( lnk=master.read ; lnk!=NULL ; lnk=lnk->next )
			linkage_master_to_slave(lnk);
		
		/* resume the main loop */
		exec_mls_resume(slave_cache->ts);

		/* wait for main loop to pause */
		exec_mls_statewait(MLS_PAUSED);

		/* @todo copy output linkages */
		for ( lnk=master.read ; lnk!=NULL ; lnk=lnk->next )
			linkage_master_to_slave(lnk);

		/* copy the next time stamp */
		slave_cache->ts = global_clock;

		/* signal the master */
		instance_slave_done();
	}
	output_verbose("slave %d completion state reached", slave_cache->id);
	return NULL;
}

/** instance_slave_wait
	Place slave in wait state until master signal it to resume
	@return 1 on success, 0 on failure.
 **/
int instance_slave_wait(void)
{
	int status = 0;
	output_debug("slave %d entering wait state", slave_id);
#ifdef WIN32
	{	DWORD rc = WaitForSingleObject(slave_eventHandle,global_signal_timeout);
		switch ( rc ) {
		case WAIT_ABANDONED:
			output_error("slave %d wait abandoned", slave_id);
			break;
		case WAIT_OBJECT_0:
			output_debug("slave %d wait completed", slave_id);
			status = 1;
			break;
		case WAIT_TIMEOUT:
			output_error("slave %d wait timeout", slave_id);
			break;
		case WAIT_FAILED:
			output_error("slave %d wait failed (error code %d)",slave_id,GetLastError());
			break;
		default:
			output_error("slave %d wait return code not recognized (rc=%d)", slave_id,rc);
			break;
		}
	}
	ResetEvent(slave_eventHandle);
#else
	// @todo linux/unix slave signalling
#endif

	/* signal main loop to resume with new timestamp */
	return status;
}

/** instance_slave_done
	Signal the master that the slave is done.
 **/
void instance_slave_done(void)
{
	output_debug("slave %d signalling master", slave_id);
#ifdef WIN32
	SetEvent(master_eventHandle);
#else
	// @todo linux/unix slave signalling
#endif
}

/** instance_slave_init
	Initialize a slave instance.
	@returns 1 on success, 0 on failure.
 **/
int instance_slave_init(void)
{
	char eventName[256];
	char cacheName[256];	
	size_t cacheSize = sizeof(MESSAGE)+MSGALLOCSZ; /* @todo size instance cache dynamically from linkage list */

	global_multirun_mode = MRM_SLAVE;
	output_prefix_enable();

#ifdef WIN32
	/* @todo open cache */
	sprintf(cacheName,"GLD-%"FMT_INT64"x",global_master_port);
	hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,cacheName);
	if ( !hMap )
	{
		output_error("unable to open cache '%s' for slave (error code %d)", cacheName, GetLastError());
		return 0;
	}
	else
		output_debug("cache '%s' opened for slave", cacheName);
	slave_cache = (void*)MapViewOfFile(hMap,FILE_MAP_ALL_ACCESS,0,0,(DWORD)cacheSize);
	if ( !slave_cache )
	{
		output_error("unable to map view of cache '%s' for slave", cacheName);
		return 0;
	}
	else
		output_debug("cache '%s' map view for slave ok (cache usage is %d of %d bytes allocated)", cacheName, slave_cache->usize, slave_cache->asize);
	message_init(slave_cache,cacheSize);
	slave_id = slave_cache->id;

	/* open slave signalling event */
	sprintf(eventName,"GLD-%"FMT_INT64"x-S", global_master_port);
	slave_eventHandle = OpenEvent(EVENT_ALL_ACCESS,FALSE,eventName);
	if ( !slave_eventHandle )
	{
		output_error("unable to open event signal '%s' for slave %d", eventName, slave_id);
		return 0;
	}
	else
		output_debug("opened event signal '%s' for slave %d", eventName, slave_id);

	/* open master signalling event */
	sprintf(eventName,"GLD-%"FMT_INT64"x-M", global_master_port);
	master_eventHandle = OpenEvent(EVENT_ALL_ACCESS,FALSE,eventName);
	if ( !master_eventHandle )
	{
		output_error("unable to open event signal '%s' for slave %d", eventName, slave_id);
		return 0;
	}
	else
		output_debug("opened event signal '%s' for slave %d", eventName, slave_id);
#else
	// @todo linux/unix slave signalling
#endif

	global_mainloopstate = MLS_PAUSED;
	output_verbose("opened slave end of master-slave comm channel for slave %d", slave_id);

	/* start the slave controller */
	if ( pthread_create(&slave_tid, NULL, instance_slaveproc, NULL) )
	{
		output_error("unable to start slave controller for slave %d", slave_id);
		return 0;
	}
	else
		output_verbose("started slave controller for slave %d", slave_id);

	// signal master that slave init is done
	instance_slave_done();

	return 1;
}
