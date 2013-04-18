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
#include <winsock2.h>
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#define SOCKET int
#define INVALID_SOCKET (-1)
#define closesocket close
#endif

#include <pthread.h>

#include "instance.h"
#include "instance_cnx.h"
#include "instance_slave.h"
#include "output.h"
#include "globals.h"
#include "random.h"
#include "exec.h"

clock_t instance_synctime = 0;

// only used for passing control between slaveproc and main threads
pthread_mutex_t mls_inst_lock;
pthread_cond_t mls_inst_signal;
int inst_created = 0;

// used to balance control between instance_master_wait_socket and instance_runproc
//pthread_mutex_t inst_sock_lock;
//pthread_cond_t inst_sock_signal;
int sock_created = 0;

#define MSGALLOCSZ 1024

void printcontent(unsigned char *data, size_t len){
	// needed for debugging, but only when things are really not cooperating. -mh
#if 0
	size_t i = 0;
	unsigned char *str = 0;

	str = (unsigned char *)malloc(len * 2 + 1);
	memset(str, 0, len * 2 + 1);
	for(i = 0; i < len; ++i){
		str[2*i] = data[i]/16 + 48;
		str[2*i+1] = data[i]%16 + 48;
		if(str[2*i] > 57)
			str[2*i] += 39;
		if(str[2*i+1] > 57)
			str[2*i+1] += 39;
	}
	output_debug("content = %s", str);
#endif
}

/** message_init
    Initialize a message cache for communication between master and slave.
	This function zeros the buffer and sets the usage to the first data
	entry (all but the header is unused).
	@returns Nothing
 **/
static void message_init(MESSAGE *msg, /**< pointer to the message cache */
						 size_t size,  /**< size of the message cache */
						 int16 name_size,	/**< size of the obj.prop names to pass up to the slave */
						 int16 data_size)	/**< size of the binary data buffer */
{
	msg->usize = sizeof(MESSAGE)-1;
	msg->asize = size;
	msg->ts = global_clock; // at the very least, start where the master starts!
	msg->hard_event = 0;
	msg->name_size = name_size;
	msg->data_size = data_size;
}

STATUS messagewrapper_init(MESSAGEWRAPPER **msgwpr,
							MESSAGE *msg){
	MESSAGEWRAPPER *wpr;
	size_t sz = sizeof(MESSAGE),
			sz2 = sizeof(MESSAGEWRAPPER);
	int64 a, b;
	b = (int64)msg;
	if(msgwpr == 0){
		output_error("messagewrapper_init(): null pointer");
		return FAILED;
	}
	wpr = *msgwpr = (MESSAGEWRAPPER *)malloc(sz2);
	if(wpr == 0){
		output_error("messagewrapper_init(): malloc failure");
		return FAILED;
	}

	wpr->msg = msg;

	wpr->name_size = &(msg->name_size);
	a = b+sz;
	wpr->name_buffer = (char *)a;

	wpr->data_size = &(msg->data_size);
	b = a + msg->name_size;
	wpr->data_buffer = (char *)b;

	return SUCCESS;
}

/////////////////////////////////////////////////////////////////////
// MASTER INSTANCE

static instance *instance_list = NULL;
int instances_count = 0;
int instances_exited = 0;

void *instance_runproc_socket(void *ptr){
	int running = 1;
	int rv = 0;
	int got_data = 0;
	instance *inst = (instance*)ptr;
	inst->has_data = 0;

	while(running){
		rv = recv(inst->sockfd, inst->buffer, (int)(inst->buffer_size), 0);
//		output_debug("%d = recv(%d, %x, %d, 0)", rv, inst->sockfd, inst->buffer, inst->buffer_size);
		if(0 == rv){
			output_error("instance_runproc_socket(): socket was closed before receiving data");
			running = 0;
		} else if(0 > rv){
			output_error("instance_runproc_socket(): error receiving data");
			running = 0;
		}
		pthread_mutex_lock(&inst->sock_lock);
		if(0 == memcmp(inst->buffer, MSG_DATA, strlen(MSG_DATA))){
			got_data = 1;
//			wlock(&inst->has_data_lock);
			inst->has_data += 1;
//			wunlock(&inst->has_data_lock);
			//output_debug("instance_runproc_socket(): found "MSG_DATA);
			//output_debug("instance_runproc_socket(): recv'd %d bytes", rv);
		} else if(0 == memcmp(inst->buffer, MSG_ERR, strlen(MSG_ERR))){
			output_error("instance_runproc_socket(): slave indicated an error occured"); // error occured
			running = 0;
		} else if(0 == memcmp(inst->buffer, MSG_DONE, strlen(MSG_DONE))){
			output_verbose("instance_runproc_socket(): slave indicated run completion"); // other side is done and is closing down
			running = 0;
		} else {
			// stdio message
			;
		}
		memcpy(inst->cache, inst->buffer+strlen(MSG_DATA), sizeof(MESSAGE));
		memcpy(inst->message->data_buffer, inst->buffer+strlen(MSG_DATA)+sizeof(MESSAGE), inst->prop_size);

		/* copy to inst->somewhere */
		//output_debug("i_rp_s(): waiting to send broadcast %d", inst->sock_signal);
		//pthread_mutex_lock(&inst->wait_lock);
//		pthread_mutex_lock(&inst->sock_lock);
//		pthread_cond_wait(&inst->wait_signal, &inst->wait_lock);
//		pthread_cond_wait(&inst->wait_signal, &inst->sock_lock);
		output_debug("inst %d sending signal 0x%x", inst->id, &(inst->sock_signal));
		pthread_cond_broadcast(&(inst->sock_signal));
//		pthread_mutex_unlock(&inst->wait_lock);
		pthread_mutex_unlock(&inst->sock_lock);
		//output_debug("i_rp_s(): sending broadcast %d", inst->sock_signal);
		
		
	}
	pthread_cond_broadcast(&inst->sock_signal);
	sock_created = 0;
	return 0;
}

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
	char1024 cmd;
	int64 rc = 0;

	if(0 == ptr){
		output_error("instance_runproc(): null instance pointer provided");
		return (void *)-1;
	}
	switch(inst->cnxtype){
		case CI_MMAP:
#ifdef WIN32
			/* run new instance */
			sprintf(cmd,"%s/gridlabd %s %s --slave %s:%"FMT_INT64"x %s &", global_execdir, global_verbose_mode?"--verbose":"", global_debug_output?"--debug":"", global_hostname,inst->cacheid, inst->model);
			output_verbose("starting new instance with command '%s'", cmd);
			rc = system(cmd);
			break;
#else
			output_error("Memory Map (mmap) instance mode only supported under Windows, please use Shared Memory (shmem) instead.");
			rc = -1;
			break;
#endif
		case CI_SHMEM:
#ifdef WIN32
			rc = -1;
			break;
#else
			// shmem not written yet
#endif
			break;
		case CI_SOCKET:
			instance_runproc_socket(ptr);
			break;
		default:
			output_error("unrecognized connection interface type in instance_runproc()");
			rc = -1;
	}

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
	
	if(0 == host){
		output_error("instance_create(): null host string provided");
		return NULL;
	}
	/* allocate memory for new instance */
	inst = (instance*)malloc(sizeof(instance));
	if ( !inst )
	{
		output_error("instance_create(): unable to allocate memory for instance on %s", host);
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
	if(0 == inst){
		output_error("instance_add_linkage(): null inst pointer");
		return 0;
	}
	if(0 == lnk){
		output_error("instance_add_linkage(): null lnk pointer");
		return 0;
	}
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

int instance_master_wait_mmap(instance *inst){
#ifdef WIN32
	int status = 0;
	DWORD rc;
	
	if(0 == inst){
		output_error("instance_master_wait_mmap(): null inst pointer");
		return status;
	}

	rc = WaitForSingleObject(inst->hMaster,global_signal_timeout);
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
	ResetEvent(inst->hMaster);
	// copy data to cache
	memcpy(inst->cache, inst->buffer, inst->cachesize);
	return status;
#else
	output_error("instance_master_wait_mmap(): should not have been called outside Windows");
	return 0;
#endif
}

int instance_master_wait_socket(instance *inst){

	if(0 == inst){
		output_error("instance_master_wait_socket(): null inst pointer");
		return 0;
	}

	if(sock_created){
		// wait for message
//		wlock(&inst->has_data_lock);
		pthread_mutex_lock(&inst->sock_lock);
		if(inst->has_data > 0){ // maybe 'while' this?
//			wunlock(&inst->has_data_lock);
			output_debug("instance_master_wait_socket(): already has data for %d", inst->id);
		} else {
			//wunlock(&inst->has_data_lock);
			//output_debug("instance_master_wait_socket(): requesting unwait on %d", inst->sock_signal);
			output_debug("instance_master_wait_socket(): inst %d waiting on %x", inst->id, &inst->sock_signal);
			//pthread_cond_broadcast(&inst->wait_signal);
			
//			pthread_mutex_lock(&inst->sock_lock);
//			pthread_cond_broadcast(&inst->wait_signal);
			output_debug("inst %d waiting on signal 0x%x", inst->id, &(inst->sock_signal));
			pthread_cond_wait(&inst->sock_signal, &inst->sock_lock);
			pthread_mutex_unlock(&inst->sock_lock);
			
		}
//		wlock(&inst->has_data_lock);
		inst->has_data -= 1;
//		wunlock(&inst->has_data_lock);
		pthread_mutex_unlock(&inst->sock_lock);
	} else {
		output_debug("instance_master_wait_socket(): no socket mutexes");
		return 0;
	}
	return 1;
}

/** instance_master_wait
	Wait the master into a wait state for all the slave->master signal.
 **/
int instance_master_wait(void)
{
	instance *inst = 0;
	int status = 0;

	for ( inst=instance_list ; inst!=NULL ; inst=inst->next )
	{
		output_verbose("master waiting on slave %d", inst->id);
#ifdef WIN32
		if(inst->cnxtype == CI_MMAP){
			status = instance_master_wait_mmap(inst);
		}
#else
	// @todo linux/unix slave signalling
#endif
		if(inst->cnxtype == CI_SOCKET){
			status = instance_master_wait_socket(inst);
		}
		if(status == 0)
			break;
//		output_verbose("slave %d resumed with t2=%lli (%x)", inst->id, inst->cache->ts, (&inst->cache->ts-inst->cache));
	}
	output_verbose("master resuming");
	return status;
}

// simple and to the point
void instance_master_done_mmap(instance *inst){
	if(0 == inst){
		output_error("instance_master_done_mmap(): null inst pointer");
		return;
	}
	//output_verbose("master signaling slave %d with timestamp %lli", inst->id, inst->cache->ts);
	
	
#ifdef WIN32
	SetEvent(inst->hSlave);
#else
	// @todo linux/unix slave signalling
#endif
}

void instance_master_done_shmem(instance *inst){
	;	// send
}

void instance_master_done_socket(instance *inst){
	int rv = 0;
	int sz = 0;
	int offset = 0;

	if(0 == inst){
		output_error("instance_master_done_socket(): null inst pointer");
		return;
	}

	// write data from cache to buffer
	memset(inst->buffer, 0, inst->buffer_size);
	sprintf(inst->buffer, MSG_DATA);
	offset = (int)strlen(MSG_DATA);
	memcpy(inst->buffer+offset, inst->cache, sizeof(MESSAGE));
	offset += sizeof(MESSAGE);
	memcpy(inst->buffer+offset, inst->message->data_buffer, *(inst->message->data_size));
	offset += *(inst->message->data_size);

	// send
	//output_debug("imds(): sending %d bytes", offset);
	rv = send(inst->sockfd, inst->buffer, offset, 0);
	printcontent(inst->buffer, rv);
	//output_debug("%d = send(%d, %x, %d, 0)", rv, inst->sockfd, inst->buffer, offset);
	if(0 == rv){
		// socket closed
		output_error("instance_master_done_socket(): inst %d closed its socket", inst->id);
		closesocket(inst->sockfd);
	} else if(0 > rv){
		// socket error
		output_error("instance_master_done_socket(): error sending data to inst %d", inst->id);
		closesocket(inst->sockfd);
	}
}

/** instance_master_done
    Signal all the slaves that the master is done processing the next sync state.
 **/
void instance_master_done(TIMESTAMP t1)
{
	instance *inst;
	for ( inst=instance_list ; inst!=NULL ; inst=inst->next )
	{
		//output_debug("master setting slave %d controller c->ts from %lli to t1 %lli", inst->cache->id, inst->cache->ts, t1);
		// needs to be done in instance_write_slave, this is too late
		inst->cache->ts = t1;
		switch(inst->cnxtype){
			case CI_MMAP:
				instance_master_done_mmap(inst);
				break;
			case CI_SHMEM:
				instance_master_done_shmem(inst);
				break;
			case CI_SOCKET:
				instance_master_done_socket(inst);
				break;
			default:
				// complain
				;
		}
	}
}



/** instance_init
    Initialize an instance object.  This occurs on the master side for each slave instance.
	return 1 on failure, 0 on success.
 **/
STATUS instance_init(instance *inst)
{
	linkage *lnk;
	size_t name_offset=0, prop_offset=0;
	STATUS result;

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
	
	if(0 == inst){
		output_error("instance_init(): null inst pointer");
		return FAILED;
	}

	/* check for looping model */
	if (strcmp(inst->model,global_modelname)==0)
	{
		output_error("instance_init(): slave instance model '%s' is the same as the master model '%'", inst->model,global_modelname);
		return FAILED;
	}

	// validate cnxtype
	if(inst->cnxtypestr[0] != 0){
		for (i = 0; i < cnxtypecnt; ++i){
			if(0 == strcmp(inst->cnxtypestr, cnxtype[i].word)){
				inst->cnxtype = cnxtype[i].type;
				break;
			}
		}
		if(i == cnxtypecnt){ // exhausted the list without finding a match
			output_error("instance_init(): unrecognized connection type '%s' for instance '%s'", inst->cnxtypestr, inst->model);
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
			output_error("instance_init(): linkage_init failed for inst '%s.%s' on link '%s.%s'", inst->hostname, inst->model, lnk->local.obj, lnk->local.prop);
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
			output_error("instance_init(): linkage_init failed for inst '%s.%s' on link '%s.%s'", inst->hostname, inst->model, lnk->local.obj, lnk->local.prop);
			return FAILED;
		} else {
			inst->cachesize += lnk->size;
			inst->prop_size += lnk->prop_size;
			inst->name_size += lnk->name_size;
			++inst->reader_count;
		}
	}

	// allocate message buffer
	inst->buffer_size = inst->cachesize = sizeof(MESSAGE) + (size_t)(inst->name_size + inst->prop_size);
	inst->cache = (MESSAGE *)malloc(inst->cachesize);
	memset(inst->cache, 0, inst->cachesize);
	message_init(inst->cache, inst->cachesize, (int16)inst->name_size, (int16)inst->prop_size);
	if(FAILED == messagewrapper_init(&(inst->message), inst->cache)){
		return FAILED;
	}
	
	//	initialize cache
	inst->cache->id = inst->id = instances_count;

	//output_verbose("inst_init(): slave %d cache at %x, ts at %x", instances_count, inst->cache, &(inst->cache->ts));

	// write property lists
	// properties are written into the buffer as "obj1.prop1,obj2.prop2 obj3.prop3,obj4.prop4\0".
	//	a space separates the writers from the readers.
	for ( lnk=inst->write ; lnk!=NULL ; lnk=lnk->next ){
		sprintf(inst->message->name_buffer+name_offset, "%s.%s%c", lnk->remote.obj, lnk->remote.prop, (lnk->next == 0 ? ' ' : ','));
		lnk->addr = (char *)(inst->message->data_buffer + prop_offset);
		name_offset += lnk->name_size;
		prop_offset += lnk->prop_size;
	}
	for ( lnk=inst->read ; lnk!=NULL ; lnk=lnk->next ){
		sprintf(inst->message->name_buffer+name_offset, "%s.%s%c", lnk->remote.obj, lnk->remote.prop, (lnk->next == 0 ? '\0' : ','));
		lnk->addr = (char *)(inst->message->data_buffer + prop_offset);
		name_offset += lnk->name_size;
		prop_offset += lnk->prop_size;
	}
	// note that for the first go-around, we cannot write the property values.
	//	the system is still in init_all() and has not gotten to object
	//	initialization, where defaults, calculated values, etc will be set.

	// create message layer
	// this will set inst->buffer to a pointer that can be used for data
	//	exchange, whether it point to shmem or to a buffer that will be
	//	fed into send()
	result = instance_connect(inst);
	if(result == FAILED){
		output_error("instance_init(): instance_connect() failed");
		return FAILED;
	}
	// start instance_proc thread
	/* start the slave instance */
	if ( pthread_create(&(inst->threadid), NULL, instance_runproc, (void*)inst) )
	{
		output_error("instance_init(): unable to starte instance slave %d", inst->id);
		return FAILED;
	}

	output_verbose("instance_init(): started instance for slave %d", inst->id);
	instances_count++;
	return SUCCESS;
}

/** instance_initall
	Initialize all instance objects.
	@note This function waits for the slaves to signal their initialization has completed using instance_slave_done().
	@see instance_slave_done
	@returns SUCCESS or FAILED.
 **/
STATUS instance_initall(void)
{
	instance *inst = 0;
	int rv;

	if ( instance_list ) 
	{
		global_multirun_mode = MRM_MASTER;
		output_verbose("entering multirun mode");
		output_prefix_enable();
		pthread_mutex_lock(&mls_inst_lock);
		pthread_cond_wait(&mls_inst_signal, &mls_inst_lock);
		
	} else {
		return SUCCESS;
	}
	for ( inst=instance_list ; inst!=NULL ; inst=inst->next )
	{
		if ( FAILED == instance_init(inst)){
			return FAILED;
		}
	}

	// wait for slaves to signal init done
	rv = instance_master_wait();
	if(0 == rv){
		output_error("instance_initall(): final wait() failed");
		return FAILED;
	}
	return SUCCESS;
}

/** instance_write_slave
    Write linkages to slaves.
 **/
STATUS instance_write_slave(instance *inst)
{
	linkage *lnk;
	STATUS res;

	if(0 == inst){
		output_error("instance_write_slave(): null inst pointer");
		return FAILED;
	}
	/* update buffer header */

	/* write output to instance */
	//output_verbose("master writing links for inst %d", inst->id);
	for ( lnk=inst->write ; lnk!=NULL ; lnk=lnk->next ){
		res = linkage_master_to_slave(0, lnk);
		if(FAILED == res){
			output_error("instance_write_slave(): linkage_master_to_slave failed");
			return FAILED;
		}
	}
	//output_verbose("copying %d bytes from %x to %x (%lli)", inst->cachesize, inst->cache, inst->buffer, inst->cache->ts);
	memcpy(inst->buffer, inst->cache, inst->cachesize);
	printcontent(inst->buffer, (int)inst->cachesize);
	return SUCCESS;
}

/** instance_read_slaves
    Read linkages from slaves (including next time)
 **/
TIMESTAMP instance_read_slave(instance *inst)
{
	linkage *lnk;
	TIMESTAMP t2 = TS_INVALID;
	STATUS res;

	if(0 == inst){
		output_error("instance_read_slave(): null inst pointer");
		return TS_INVALID;
	}

//	printcontent(inst->buffer, (int)inst->cachesize);
	t2 = inst->cache->ts;

	//output_debug("master reading links for inst %d", inst->id);
	/* @todo read input from instance */
	for ( lnk=inst->read ; lnk!=NULL ; lnk=lnk->next ){
		res = linkage_slave_to_master(0, lnk);
		if(FAILED == res){
			;
		}
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
	int wc = 0;
	/* only process if instances exist */
	if ( instance_list )
	{
		TIMESTAMP t2 = TS_NEVER;
		clock_t ts = exec_clock();
		instance *inst;

		/* check to see if an instance was lost */
		if ( instances_exited>0 )
		{
			output_debug("%d instance exit detected, stopping main loop", instances_exited);
	
			/* tell main too to stop */
			return TS_INVALID;
		}
	
		/* send linkage to slaves */
		for ( inst=instance_list ; inst!=NULL ; inst=inst->next ){
			inst->cache->ts = t1;
			instance_write_slave(inst);
		}
	
		/* signal slaves to start */
		instance_master_done(t1);
	
		wc = instance_master_wait();
		if(wc != 0){
			;
		}

		/* read linkages from slaves */
		for ( inst=instance_list ; inst!=NULL ; inst=inst->next )
		{
			TIMESTAMP t3 = 0;
			
			t3 = instance_read_slave(inst);
			if ( t3 < t2 ){
				t2 = t3;
			}
		}
	
		output_debug("instance sync time is %"FMT_INT64"d", t2);
		instance_synctime += exec_clock() - ts;
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

// see "instance_slave.c" -mh

// EOF
