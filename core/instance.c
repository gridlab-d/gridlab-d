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
extern struct sync_data sync;

// only used for passing control between slaveproc and main threads
pthread_mutex_t mls_inst_lock;
pthread_cond_t mls_inst_signal;
int inst_created = 0;

#define MSGALLOCSZ 1024

void printcontent(char *data, int len){
	/*int i = 0;
	char a, b;
	char *str;

	str = (char *)malloc(len * 2 + 1);
	memset(str, 0, len * 2 + 1);
	for(i = 0; i < len; ++i){
		str[2*i] = data[i]/16 + 48;
		str[2*i+1] = data[i]%16 + 48;
		if(str[2*i] > 57)
			str[2*i] += 39;
		if(str[2*i+1] > 57)
			str[2*i+1] += 39;
	}
	output_debug("content = %s", str);*/
}

/** message_init
    Initialize a message cache for communication between master and slave.
	This function zeros the buffer and sets the usage to the first data
	entry (all but the header is unused).
	@returns Nothing
 **/
static void message_init(MESSAGE *msg, /**< pointer to the message cache */
						 size_t size,  /**< size of the message cache */
						 bool clear,		/**< flag to memset bytes after the struct (master only!) */
						 int16 name_size,	/**< size of the obj.prop names to pass up to the slave */
						 int16 data_size)	/**< size of the binary data buffer */
{
	msg->usize = sizeof(MESSAGE)-1;
	msg->asize = size;
	msg->ts = global_clock; // at the very least, start where the master starts!
	msg->hard_event = 0;
	msg->name_size = name_size;
	msg->data_size = data_size;
	if(clear)
		memset(&(msg->data),0,size);
}

STATUS messagewrapper_init(MESSAGEWRAPPER **msgwpr,
							MESSAGE *msg){
	MESSAGEWRAPPER *wpr;
	size_t sz = sizeof(MESSAGE);
	int64 a, b;
	b = (int64)msg;
	if(msgwpr == 0){
		output_error("messagewrapper_init(): null pointer");
		return FAILED;
	}
	wpr = *msgwpr = (MESSAGEWRAPPER *)malloc(sizeof(MESSAGEWRAPPER));
	if(wpr == 0){
		output_error("messagewrapper_init(): malloc failure");
		return FAILED;
	}

	wpr->msg = msg;

	wpr->name_size = &(msg->name_size);
	a = b+sz;
//	wpr->name_buffer = (char *)(msg + sz);
	wpr->name_buffer = (char *)a;

	wpr->data_size = &(msg->data_size);
//	wpr->data_buffer = (char *)(msg + sz + msg->name_size);
	b = a + msg->name_size;
	wpr->data_buffer = (char *)b;

	return SUCCESS;
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
	return NULL;
	// nuked.  -mh
#if 0
	MESSAGE *msg;
	if (size==0) size = sizeof(MESSAGE)+MSGALLOCSZ;
	msg = malloc(size);
	if ( !msg ) return NULL;

	/* initialize message */
	message_init(msg,size,true, 0, 0);
	output_fatal("message cache be created using existing code");
	return msg;
#endif
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
	return NULL;
	// nuked.  -mh
#if 0
	size_t size = old->asize+MSGALLOCSZ;
	MESSAGE *msg = message_new(size);
	free(old);
	output_fatal("message cache cannot grow using existing code");
	return msg;
#endif
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
	//size_t pos, need;

	return NULL;
	// nuked. -mh
#if 0
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
#endif
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
//	exec_mls_resume(TS_NEVER);
//	pthread_cond_broadcast(&mls_inst_signal);
	
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
		SetEvent(inst->hSlave);
#else
	// @todo linux/unix slave signalling
#endif
	}
}

STATUS instance_cnx_mmap(instance *inst){
#ifdef WIN32
		char cachename[1024];
		char eventname[64];
		SECURITY_ATTRIBUTES secAttr = {sizeof(SECURITY_ATTRIBUTES),(LPSECURITY_ATTRIBUTES)NULL,TRUE}; 

		if(inst == 0){
			output_error("instance_cnx_mmap: no instance provided");
			/*	TROUBLESHOOT
				There was an internal error that was not caught prior to attempting to construct
				the message-passing layer without an instance for context.
				*/
			return FAILED;
		}

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
				return FAILED;
			}
			else
			{
				output_debug("cache '%s' created for instance '%s'", cachename, inst->model);
			}
		}
		else
		{
			output_error("cache '%s' for instance '%s' already in use", cachename, inst->model);
			return FAILED;
		}
		inst->buffer = (char *)MapViewOfFile(inst->hMap,FILE_MAP_ALL_ACCESS,0,0,(DWORD)inst->cachesize);
		if ( !inst->buffer )
		{
			output_error("unable to map view of cache '%s' for instance '%s' already in use", cachename, inst->model);
			return FAILED;
		}
		output_debug("cache '%s' map view for model '%s' ok", cachename, inst->model);
		output_debug("resulting handle is %x", inst->buffer);
		/* initialize slave instance cache */
		//message_init(inst->cache,inst->cachesize,true); // I know this isn't needed here -MH
		inst->cache->id = inst->id = instances_count;
		output_verbose("slave %d assigned to '%s'", inst->id, inst->model);
		output_debug("slave %d cache size is %d of %d bytes allocated", inst->id, inst->cache->usize, inst->cache->asize);

		/* copy existing message buffer to cache */
		output_debug("copying %d bytes from %x to %x", inst->cachesize, inst->cache, inst->buffer);
		memcpy(inst->buffer, inst->cache, inst->cachesize);

		/* setup master signalling event */
		sprintf(eventname,"GLD-%"FMT_INT64"x-M", inst->cacheid);
		inst->hMaster = CreateEvent(&secAttr,FALSE,FALSE,eventname); /* initially unsignalled */
		if ( !(inst->hMaster) )
		{
			output_error("unable to create event signal '%s' for slave %d", eventname, inst->id);
			return 1;
		}
		else
		{
			output_debug("created event signal '%s' for slave%d ", eventname, inst->id);
		}

		/* setup slave signalling event */
		sprintf(eventname,"GLD-%"FMT_INT64"x-S", inst->cacheid);
		inst->hSlave = CreateEvent(&secAttr,FALSE,FALSE,eventname); /* initially unsignalled */
		if ( !(inst->hSlave) )
		{
			output_error("unable to create event signal '%s' for slave %d", eventname, inst->id);
			return FAILED;
		}
		else
		{
			output_debug("created event signal '%s' for slave %d", eventname, inst->id);
		}
		return SUCCESS;
#else
	output_error("Memory Map (mmap) instance mode only supported under Windows, please use Shared Memory (shmem) instead.");
	return FAILED;
#endif
}

STATUS instance_cnx_shmem(instance *inst){
	return FAILED;
}

STATUS instance_cnx_socket(instance *inst){
	// note: this needs to thread off an accept()'ing socket in order to
	//	handle the initialization process.  multiple slaves may introduce
	//	'difficulties' with port use and reuse.
	return FAILED;
}

/** instance_init
    Initialize an instance object.  This occurs on the master side for each slave instance.
	return 1 on failure, 0 on success.
 **/
STATUS instance_init(instance *inst)
{
	linkage *lnk;
//	int16 *a, *b;
	size_t name_offset=0, prop_offset=0;
//	char *buffer;

	int i;
	int cnxtypecnt = 3;
	struct {
		char *word;
		CNXTYPE type;
	} cnxtype[] = {
		{"mmap", CI_MMAP},
		{"shmem", CI_SHMEM},
		{"socket", CI_SOCKET},
	};

	/* check for looping model */
	if (strcmp(inst->model,global_modelname)==0)
	{
		output_error("slave instance model '%s' is the same as the master model '%'", inst->model,global_modelname);
		return 1;
	}

	/*** REHASH OF PSUEDOCODE AND REQUIREMENTS ***/
	// validate cnxtype
	if(inst->cnxtypestr[0] != 0){
		for (i = 0; i < cnxtypecnt; ++i){
			if(0 == strcmp(inst->cnxtypestr, cnxtype[i].word)){
				inst->cnxtype = cnxtype[i].type;
				break;
			}
		}
		if(i == cnxtypecnt){ // exhausted the list without finding a match
			output_error("unrecognized connection type '%s' for instance '%s'", inst->cnxtypestr, inst->model);
			return FAILED;
		}
	} else {
		// default
#ifdef WIN32
		inst->cnxtype = CI_MMAP;
#else
		inst->cnxtype = CI_SHMEM;
#endif
	}
	// calculate message buffer requirements
	/* initialize linkages */
	inst->cachesize = sizeof(MESSAGE);
	inst->prop_size = 0;
	inst->name_size = 0;

	for ( lnk=inst->write ; lnk!=NULL ; lnk=lnk->next ){
		if ( linkage_init(inst, lnk)==FAILED ){
			output_error("linkage_init failed for inst '%s.%s' on link '%s.%s'", inst->hostname, inst->model, lnk->local.obj, lnk->local.prop);
			return FAILED;
		} else {
			inst->cachesize += lnk->size;
			inst->prop_size += lnk->prop_size;
			inst->name_size += lnk->name_size;
			++inst->writer_count;
		}
	}
	for ( lnk=inst->read ; lnk!=NULL ; lnk=lnk->next ){
		if ( linkage_init(inst, lnk)==FAILED ){
			output_error("linkage_init failed for inst '%s.%s' on link '%s.%s'", inst->hostname, inst->model, lnk->local.obj, lnk->local.prop);
			return FAILED;
		} else {
			inst->cachesize += lnk->size;
			inst->prop_size += lnk->prop_size;
			inst->name_size += lnk->name_size;
			++inst->reader_count;
		}
	}

	// allocate message buffer
	/* The rationale is that there needs to be a local buffer that can be safely
	 *	written to and read from by the local instance for a given master-slave
	 *	relationship without needing to tangle with cross-process locking.
	 * Furthermore, local memory is cheap and more easily allocated than shared
	 *	memory. -MH
	 */
#if 0
	inst->buffer = malloc(inst->cachesize);
	if(inst->buffer == 0){
		output_error("malloc failure in instance_init for message buffer ~ %d bytes for %s.%s", inst->cachesize, inst->hostname, inst->model);
		return FAILED;
	}
	memset(inst->buffer, 0, /*sizeof(MESSAGE) + */ inst->cachesize);
#endif
	//messagewrapper_init(&(inst->message), inst->name_size, inst->prop_size);
	//inst->cache = &(inst->message->msg);

	inst->cachesize = sizeof(MESSAGE) + (size_t)(inst->name_size + inst->prop_size);
	inst->cache = (MESSAGE *)malloc(inst->cachesize);
	memset(inst->cache, 0, inst->cachesize);
	message_init(inst->cache, inst->cachesize, true, inst->name_size, inst->prop_size);
	if(FAILED == messagewrapper_init(&(inst->message), inst->cache)){
		return FAILED;
	}


	// write property lists
	// properties are written into the buffer as "obj1.prop1,obj2.prop2 obj3.prop3,obj4.prop4\0".
	//	a space separates the writers from the readers.
	for ( lnk=inst->write ; lnk!=NULL ; lnk=lnk->next ){
		sprintf(inst->message->name_buffer+name_offset, "%s.%s%c", lnk->remote.obj, lnk->remote.prop, (lnk->next == 0 ? ' ' : ','));
		lnk->addr = (char *)(inst->message->data_buffer + prop_offset);
		name_offset += lnk->name_size;
		prop_offset += lnk->prop_size;
		output_debug("lnk addr %x, val %s.%s%c",lnk->addr, lnk->remote.obj, lnk->remote.prop, (lnk->next == 0 ? ' ' : ','));
	}
	for ( lnk=inst->read ; lnk!=NULL ; lnk=lnk->next ){
		sprintf(inst->message->name_buffer+name_offset, "%s.%s%c", lnk->remote.obj, lnk->remote.prop, (lnk->next == 0 ? '\0' : ','));
		lnk->addr = (char *)(inst->message->data_buffer + prop_offset);
		name_offset += lnk->name_size;
		prop_offset += lnk->prop_size;
	}
	output_debug("propstr \'%s\'", inst->message->name_buffer);
	// note that for the first go-around, we cannot write the property values.
	//	the system is still in init_all() and has not gotten to object
	//	initialization, where defaults, calculated values, etc will be set.

	// create message layer
	// this will set inst->buffer to a pointer that can be used for data
	//	exchange, whether it point to shmem or to a buffer that will be
	//	fed into send()
	switch(inst->cnxtype){
		case CI_MMAP:
			if(FAILED == instance_cnx_mmap(inst)){
				output_error("unable to create memory map for instance %s.%s", inst->hostname, inst->model);
				return FAILED;
			}
			break;
		case CI_SHMEM:
			if(FAILED == instance_cnx_shmem(inst)){
				output_error("unable to create memory map for instance %s.%s", inst->hostname, inst->model);
				return FAILED;
			}

			break;
		case CI_SOCKET:
			if(FAILED == instance_cnx_socket(inst)){
				output_error("unable to create memory map for instance %s.%s", inst->hostname, inst->model);
				return FAILED;
			}
			break;
		default:
			output_error("instance %s.%s contains invalid connection type", inst->hostname, inst->model);
			return FAILED;
			break;
	}

	// start instance_proc thread
	/* start the slave instance */
	if ( pthread_create(&(inst->threadid), NULL, instance_runproc, (void*)inst) )
	{
		output_error("unable to starte instance slave %d", inst->id);
		return FAILED;
	}
	else
	{
		output_verbose("started instance for slave %d", inst->id);
		instances_count++;
		return SUCCESS;
	}

	/*** END REHASH ***/

	return SUCCESS;
	// short circuit'ed below this point
#if 0
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
		message_init(inst->cache,inst->cachesize,true);
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
	// TODO: write 'remote' object and property names into each instance's linkages.
	// also need to write "coming" or "going"...
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
#endif
}

/** instance_initall
	Initialize all instance objects.
	@note This function waits for the slaves to signal their initialization has completed using instance_slave_done().
	@see instance_slave_done
	@returns SUCCESS or FAILED.
 **/
STATUS instance_initall(void)
{
	instance *inst;
	if ( instance_list ) 
	{
		global_multirun_mode = MRM_MASTER;
		output_verbose("entering multirun mode");
		output_prefix_enable();
		pthread_mutex_lock(&mls_inst_lock);
		pthread_cond_wait(&mls_inst_signal, &mls_inst_lock);
		
	}
	for ( inst=instance_list ; inst!=NULL ; inst=inst->next )
	{
		if ( instance_init(inst) == FAILED){
			return FAILED;
		}
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
	for ( lnk=inst->write ; lnk!=NULL ; lnk=lnk->next ){
		linkage_master_to_slave(inst, lnk);
	}
	// @todo "transfer data to cnx medium"
	output_debug("copying %d bytes from %x to %x", inst->cachesize, inst->cache, inst->buffer);
	memcpy(inst->buffer, inst->cache, inst->cachesize);
	printcontent(inst->buffer, inst->cachesize);
}

/** instance_read_slaves
    Read linkages from slaves (including next time)
 **/
TIMESTAMP instance_read_slave(instance *inst)
{
	TIMESTAMP t2 = TS_INVALID;
	linkage *lnk;

	// @todo "transfer data from cnx medium"
output_debug("copying %d bytes from %x to %x", inst->cachesize, inst->buffer, inst->cache);
	memcpy(inst->cache, inst->buffer, inst->cachesize);
	printcontent(inst->buffer, inst->cachesize);
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
			memcpy(inst->cache, inst->buffer, inst->cachesize);
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
	for ( lnk=inst->read ; lnk!=NULL ; lnk=lnk->next ){
		linkage_slave_to_master(inst, lnk);
	}

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
			if ( t3 < t2 ){
				t2 = t3;
			}
			sync.hard_event += inst->cache->hard_event;
		}
	
		output_debug("instance sync time is %"FMT_INT64"d", t2);
		instance_synctime += clock() - ts;
		return t2;
	}
	else
		return TS_NEVER;
}

STATUS instance_dispose(){
	instance *inst = 0;
	if(instance_list){ // master
		instance_master_done(TS_NEVER);
		for(inst = instance_list; inst != 0; inst = inst->next){
			// release pthread and event resources
			;
		}
		return SUCCESS;
	} else { // slave
		//release pthread and event resources
		return SUCCESS;
	}
}
/////////////////////////////////////////////////////////////////////
// SLAVE INSTANCE

static MESSAGE *slave_cache;
#ifdef WIN32
//static HANDLE hMap;
//static HANDLE local_inst.hSlave;
//static HANDLE local_inst.hMaster;
#else
	// @todo linux/unix slave signalling
#endif
unsigned int slave_id;
pthread_t slave_tid;
//static struct {
//	MESSAGEWRAPPER wpr;
//	linkage *read, *write;
//} master;
static instance local_inst;

STATUS instance_slave_get_data(void *buffer, size_t offset, size_t sz){
	if(offset < 0){
		output_error("negative offset in instance_slave_get_data()");
		return FAILED;
	}
	if(sz < 0){
		output_error("negative size in instance_slave_get_data()");
		return FAILED;
	}

	switch(global_multirun_connection){
		case MRC_MEM:
#if WIN32
			if(slave_cache == 0){
				output_error("instance_slave_get_data() called with uninitialized slave_cache");
				return FAILED;
			}
			memcpy(buffer, slave_cache+offset, sz);
#else
			output_error("instance_slave_get_data() not ready for linux");
			return FAILED;
#endif
			break;
		default:
			output_error("instance_slave_get_data() called while global_multirun_connection in an unrecognized state");
			return FAILED;
	}
	return SUCCESS;
}

/** instance_slave_get_header()
	Reads a MESSAGE header off the exchange medium, copies it into a temporary buffer,
	instantiates a new full-sized message that should hold everything, then initializes
	the values for a MESSAGEWRAPPER.
 **/
STATUS instance_slave_get_header(){
	// read first sizeof(MESSAGE) bytes from message buffer
	// 1. copy data from medium to buffer (wait, is there a buffer?)
	// 2. copy data from buffer into message/wrapper
	// 3. validate data
	// 4. construct prop & data buffers
	STATUS rv;
	MESSAGE msg;
	rv = instance_slave_get_data(&msg, 0, sizeof(MESSAGE));
	if(rv == FAILED){
		return FAILED;
	}
	if(msg.name_size < 0){
		return FAILED;
	}
	if(msg.data_size < 0){
		return FAILED;
	}
	/*
	slave_cache = (MESSAGE *)malloc(msg.asize);
	memcpy(slave_cache, &msg, sizeof(MESSAGE));
	master.wpr->msg = slave_cache;
	master.wpr->name_size = &(slave_cache.name_size);
	master.wpr->name_buffer = (char *)(slave_cache + sizeof(MESSAGE));
	master.wpr->data_size = &(slave_cache.data_size);
	master.wpr->data_buffer = (char *)(slave_cache + sizeof(MESSAGE) + msg.name_size);
	*/
	return SUCCESS;
}

STATUS instance_slave_parse_prop_list(char *line, linkage **root, LINKAGETYPE type){
	char *token = 0;
	char objname[64], propname[64];
	OBJECT *obj = 0;
	PROPERTY *prop = 0;
	linkage *link = 0, *end = 0;
	int tokenct = 0;

	if(line == 0){
		return FAILED;
	}
	if(root == 0){
		return FAILED;
	}

	output_debug("parser list: \'%s\'", line);

	token = strtok(line, ", \0\n\r");	// @todo should use strtok_r, will change later
	while(token != 0){
		tokenct = sscanf(token, "%[A-Za-z0-9_].%[A-Za-z0-9_]", objname, propname);
		if(2 != tokenct){
			// @todo check for global properties instead
			output_error("instance_slave_link_properties: unable to parse '%s' (ct = %d)", token, tokenct);
			return FAILED;
		}
		obj = object_find_name(objname);
		if(obj == 0){
			output_error("instance_slave_link_properties: unable to find object '%s'", objname);
			return FAILED;
		}
		prop = object_get_property(obj, propname);
		if(prop == 0){
			output_error("instance_slave_link_properties: prop '%s' not found in object '%s'", propname, objname);
			return FAILED;
		}
		output_debug("found links for %s", token);
		// build link
		link = (linkage *)malloc(sizeof(linkage));
		if(0 == link){
			output_error("instance_slave_link_properties: malloc failure for linkage");
			return FAILED;
		}
		output_debug("making link for \'%s.%s\'", objname, propname);
		strcpy(link->remote.obj, objname);
		strcpy(link->remote.prop, propname);
		link->target.obj = obj;
		link->target.prop = prop;
		link->type = type;
		link->next = 0;
		
		link->prop_size = property_minimum_buffersize(link->target.prop);
		link->name_size = strlen(token)+1; // +1 since there was a comma or space trimmed off
		link->size = link->name_size + link->prop_size;

		if(end == 0){
			*root = end = link;
		} else {
			end->next = link;
			end = link;
		}

		// repeat
		token = strtok(NULL, ", \0\n\r");
	}
	return SUCCESS;
}

/** instance_slave_link_properties
 **/
STATUS instance_slave_link_properties(){
	char *buffer = (char *)malloc(local_inst.name_size);
//	char *work, *write;
	char *read;
	OBJECT *obj = 0;
	PROPERTY *prop = 0;
	STATUS rv;
	size_t offset = 0;
	linkage *link = 0;
	size_t maxlen = (size_t)*(local_inst.message->name_size);

	// copy to preserve the original buffer
	strcpy(buffer, local_inst.message->name_buffer);

	// split the string manually
	read = strchr(buffer, ' ');
	*read++ = 0;

	// parse writers (first half of the original string)
	// a.b,c.d,e.f' '
	output_debug("parsing writer list");
	rv = instance_slave_parse_prop_list(buffer, &(local_inst.write), LT_MASTERTOSLAVE);

	// parse readers (second half of the original string)
	// a.b,c.d,e.f'\0'
	output_debug("parsing reader list");
	rv = instance_slave_parse_prop_list(read, &(local_inst.read), LT_SLAVETOMASTER);

	// iterate through writers to determine memory offsets
	//offset = local_inst.cache->name_size + sizeof(MESSAGE);
	offset = 0;
	output_debug("li.write = %x", local_inst.write);
	for(link = local_inst.write; link != 0; link = link->next){
		// linkage_init already done in parse_prop_list
		//link->addr = local_inst.cache + offset;
		//output_debug("addr %x = %x + %x", link->addr, local_inst.cache, offset);
		link->addr = local_inst.message->data_buffer + offset;
		output_debug("addr %x = %x + %x", link->addr, local_inst.message->data_buffer, offset);
		offset += link->prop_size;
		// instance incrementers
		local_inst.prop_size += link->prop_size;
		local_inst.name_size += link->name_size;
		++local_inst.writer_count;
	}
	// iterate through readers to determine memory offsets
	output_debug("li.read = %x", local_inst.read);
	for(link = local_inst.read; link != 0; link = link->next){
		link->addr = local_inst.message->data_buffer + offset;
		output_debug("addr %x = %x + %x", link->addr, local_inst.message->data_buffer, offset);
		offset += link->prop_size;
		// instance incrementers
		local_inst.prop_size += link->prop_size;
		local_inst.name_size += link->name_size;
		++local_inst.reader_count;
	}
	return SUCCESS;
}

/** instance_slave_read_properties()
 **/
STATUS instance_slave_read_properties(){
	linkage *link;
	for(link = local_inst.write; link != 0; link = link->next){
		;
	}
	return SUCCESS;
}

/** instance_slaveproc
    Create main slave control loop to maintain sync with master.
	Anything that is dependant on objects being loaded happens here.
 **/
void *instance_slaveproc(void *ptr)
{
	linkage *lnk;
	STATUS rv;
	output_debug("slave %d controller startup in progress", slave_id);
	pthread_mutex_lock(&mls_inst_lock);
	pthread_cond_wait(&mls_inst_signal, &mls_inst_lock);
	pthread_mutex_unlock(&mls_inst_lock);
//	sync.step_to = local_inst.cache->ts;
//	output_debug("slave %d controller received first signal with gc=%lli", slave_id, sync.step_to);
	output_debug("linking properties");
	rv = instance_slave_link_properties();
	output_debug("reading properties");
	rv = instance_slave_read_properties();
	output_debug("done reading properties");
	while (global_clock!=TS_NEVER)
	{
		/* wait for master to signal slave */
		output_debug("slave %d controller waiting", slave_id);
		if ( !instance_slave_wait() )
		{
			/* stop the main loop and exit the slave controller */
			output_error("slave %d controller wait failure, thread stopping", slave_id);
//			exec_mls_resume(TS_NEVER);
			pthread_cond_broadcast(&mls_inst_signal);
			//exec_mls_done();
			break;
		}

		/* copy inbound linkages */
		output_debug("slave %d controller reading links", slave_id);
		//output_debug("li.read = %x", local_inst.read);
		// @todo cnx exchange
		output_debug("copying %d bytes from %x to %x", local_inst.cachesize, local_inst.filemap, local_inst.cache);
		memcpy(local_inst.cache, local_inst.filemap, local_inst.cachesize);
		printcontent(local_inst.filemap, local_inst.cachesize);
		for ( lnk=local_inst.write ; lnk!=NULL ; lnk=lnk->next ){
			//output_debug("reading link");
			output_debug("reading %s.%s link (buffer %x)", lnk->target.obj->name, lnk->target.prop->name, local_inst.cache);
			linkage_master_to_slave(&local_inst, lnk);
		}
		output_debug("continuing");
		
		/* resume the main loop */
		// note, if TS_NEVER, we want the slave's exec loop to end normally
		output_debug("slave %d controller resuming exec with %lli", slave_id, local_inst.cache->ts);
//		exec_mls_resume(slave_cache->ts);
		sync.step_to = local_inst.cache->ts;
		if(local_inst.cache->ts != TS_NEVER){
			sync.hard_event = 1;
		}
		pthread_cond_broadcast(&mls_inst_signal);
		if(local_inst.cache->ts == TS_NEVER){
			break;
		}

		/* wait for main loop to pause */
		output_debug("slave %d controller waiting for main to complete", slave_id);
//		exec_mls_statewait(MLS_PAUSED);
		pthread_mutex_lock(&mls_inst_lock);
		pthread_cond_wait(&mls_inst_signal, &mls_inst_lock);
		pthread_mutex_unlock(&mls_inst_lock);

		/* @todo copy output linkages */
		output_debug("slave %d controller writing links", slave_id);
		for ( lnk=local_inst.read ; lnk!=NULL ; lnk=lnk->next ){
			linkage_slave_to_master(&local_inst, lnk);
		}
		output_debug("continuing");

		/* copy the next time stamp */
		//slave_cache->ts = global_clock;
		/* how about we copy the time we want to step to and see what the master says, instead? -MH */
		local_inst.cache->ts = sync.step_to;
		local_inst.cache->hard_event = sync.hard_event;

		// write cache to cnx medium
		// this works for MMAP, needs function with switches (or struct w/ func* )
		output_debug("copying %d bytes from %x to %x", local_inst.cachesize, local_inst.cache, local_inst.filemap);
		memcpy(local_inst.filemap, local_inst.cache, local_inst.cachesize);
		printcontent(local_inst.filemap, local_inst.cachesize);

		/* signal the master */
		output_debug("slave %d controller sending 'done' signal with ts %lli/%lli", slave_id, local_inst.cache->ts, global_clock);
		instance_slave_done();
	}
	output_verbose("slave %d completion state reached", local_inst.cache->id);
	//return NULL;
	pthread_exit(NULL);
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
	{	DWORD rc = WaitForSingleObject(local_inst.hSlave,global_signal_timeout);
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
	ResetEvent(local_inst.hSlave);
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
	output_debug("slave %d signaling master", slave_id);
#ifdef WIN32
	SetEvent(local_inst.hMaster);
#else
	// @todo linux/unix slave signalling
#endif
}


/** do docx here
 **/
STATUS instance_slave_init_mem(){
#ifdef WIN32
	MESSAGE tmsg;
	char eventName[256];
	char cacheName[256];
	int rv = 0;
	size_t sz = sizeof(MESSAGE);
	size_t cacheSize;

	/* @todo open cache */
	sprintf(cacheName,"GLD-%"FMT_INT64"x",global_master_port);
	local_inst.hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,cacheName);
	if ( 0 == local_inst.hMap )
	{
		output_error("unable to open cache '%s' for slave (error code %d)", cacheName, GetLastError());
		return FAILED;
	}
	else
	{
		output_debug("cache '%s' opened for slave", cacheName);
	}
	local_inst.filemap = (void*)MapViewOfFile(local_inst.hMap,FILE_MAP_ALL_ACCESS,0,0,(DWORD)0); // not sure how big it is, so grab it all
	output_debug("MapViewOfFile returned");
	output_debug("resulting handle is %x", local_inst.filemap);
	rv = local_inst.filemap;
//	if ( output_debug("checking result"),0 == rv )
//	{
//		output_error("unable to map view of cache '%s' for slave", cacheName);
//		return FAILED;
//	}
//	else
//	{
//		output_debug("cache '%s' map view for slave ok (cache usage is %d of %d bytes allocated)", cacheName, slave_cache->usize, slave_cache->asize);
//	}

	output_debug("copying %d bytes from map %x to local message", sz, local_inst.filemap);
	memcpy(&tmsg, local_inst.filemap, sz);
	
	output_debug("data copied");
	output_debug("namesz %d, datesz %d", tmsg.name_size, tmsg.data_size);
	output_debug("TMSG: usize %d, asize %d, id %x, nsz %d, psz %d", tmsg.usize, tmsg.asize, tmsg.id, tmsg.name_size, tmsg.data_size);
	if(tmsg.name_size < 0){
		return FAILED;
	}
	if(tmsg.data_size < 0){
		return FAILED;
	}
	
	// initialize buffer/cache
	local_inst.cachesize = cacheSize = tmsg.asize;
	local_inst.buffer = (char *)malloc(local_inst.cachesize);
	local_inst.cache = (MESSAGE *)malloc(local_inst.cachesize);
	local_inst.id = slave_id = tmsg.id;

	//message_init(inst.cache,tmsg.asize,true,tmsg.name_size,tmsg.data_size);

	// THIS COPIES THE DATA
	memcpy(local_inst.cache, local_inst.filemap, local_inst.cachesize);
	messagewrapper_init(&(local_inst.message), local_inst.cache);

	sync.step_to = local_inst.cache->ts;

	/* open slave signalling event */
	sprintf(eventName,"GLD-%"FMT_INT64"x-S", global_master_port);
	local_inst.hSlave = OpenEvent(EVENT_ALL_ACCESS,FALSE,eventName);
	if ( !local_inst.hSlave )
	{
		output_error("unable to open event signal '%s' for slave %d", eventName, slave_id);
		return 0;
	}
	else
	{
		output_debug("opened event signal '%s' for slave %d", eventName, slave_id);
	}

	/* open master signalling event */
	sprintf(eventName,"GLD-%"FMT_INT64"x-M", global_master_port);
	local_inst.hMaster = OpenEvent(EVENT_ALL_ACCESS,FALSE,eventName);
	if ( !local_inst.hMaster )
	{
		output_error("unable to open event signal '%s' for slave %d", eventName, slave_id);
		return FAILED;
	}
	else
	{
		output_debug("opened event signal '%s' for slave %d ", eventName, slave_id);
	}
	return SUCCESS;
#else
	// @todo linux/unix slave signalling
#endif
}

/** do docx here
 **/
STATUS instance_slave_init_socket(){
	// must handshake by connecting and sending 'just' a message
	//	struct.  proper response is a message struct with the same ID
	//	and nonzero sizes.  ack with same struct, after malloc'ing
	//	buffer space.  response after should contain names.  responses
	//	after that will be data messages.
	return FAILED;
}

/** do docx here
 **/
STATUS instance_slave_init_pthreads(){
	int rv = 0;
	//	global_mainloopstate = MLS_PAUSED;
	rv = pthread_mutex_init(&mls_inst_lock,NULL);
	if(rv != 0){
		output_error("error with pthread_mutex_init() in instance_slave_init_pthreads()");
		return FAILED;
	}
	rv = pthread_cond_init(&mls_inst_signal,NULL);
	if(rv != 0){
		output_error("error with pthread_cond_init() in instance_slave_init_pthreads()");
		return FAILED;
	}
	output_verbose("opened slave end of master-slave comm channel for slave %d", slave_id);

//	exec_mls_create(); // need to do this before we start the thread

	/* start the slave controller */
	if ( pthread_create(&(local_inst.pid), NULL, instance_slaveproc, NULL) )
	{
		output_error("unable to start slave controller for slave %d", slave_id);
		return FAILED;
	}
	else
	{
		output_verbose("started slave controller for slave %d", slave_id);
	}
	return SUCCESS;
}

/** instance_slave_init
	Initialize a slave instance.
	NOTE, the associated GLM file has NOT been loaded at this point in time!
	@returns SUCCESS on success, FAILED on failure.
 **/
STATUS instance_slave_init(void)
{
	int rv = 0;
	STATUS stat;
	size_t cacheSize = sizeof(MESSAGE)+MSGALLOCSZ; /* @todo size instance cache dynamically from linkage list */

	global_multirun_mode = MRM_SLAVE;
	output_prefix_enable();

	// this is where we open a connection, snag the first header, and initialize everything from there
	switch(global_multirun_connection){
		case MRC_MEM:
			rv = instance_slave_init_mem();
			break;
		case MRC_SOCKET:
			rv = instance_slave_init_socket();
			break;
		case MRC_NONE:
			rv = FAILED;
			output_error("Not running in slave mode, or multirun_connection not set (internal error)");
			break;
		default:
			rv = FAILED;
			output_error("Unrecognized multirun_connection type (internal error)");
			break;
	}
	if(rv == FAILED){
		output_error("unable to initialize communication medium");
		return FAILED;
	}

	rv = instance_slave_init_pthreads(); // starts slaveproc() thread

	global_clock = sync.step_to; // copy time signal to gc, legit since it's from msg
	
	// signal master that slave init is done
	instance_slave_done();

	return SUCCESS;
}
