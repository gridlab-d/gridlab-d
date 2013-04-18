#include "instance_slave.h"

// in practice, these are initialized by instance.c
extern clock_t instance_synctime;

// only used for passing control between slaveproc and main threads
extern pthread_mutex_t mls_inst_lock;
extern pthread_cond_t mls_inst_signal;
extern int inst_created;

#define MSGALLOCSZ 1024

/////////////////////////////////////////////////////////////////////
// SLAVE INSTANCE

static MESSAGE *slave_cache;
unsigned int slave_id;
pthread_t slave_tid;
static instance local_inst;

STATUS instance_slave_get_data(void *buffer, size_t offset, size_t sz){
	if(0 == buffer){
		output_error("instance_slave_get_data(): null buffer pointer");
	}
	if(offset < 0){
		output_error("instance_slave_get_data(): negative offset");
		return FAILED;
	}
	if(sz < 0){
		output_error("instance_slave_get_data(): negative size");
		return FAILED;
	}

	switch(global_multirun_connection){
		case MRC_MEM:
#if WIN32
			if(slave_cache == 0){
				output_error("instance_slave_get_data(): called with uninitialized slave_cache");
				return FAILED;
			}
			memcpy(buffer, slave_cache+offset, sz);
#else
			output_error("instance_slave_get_data() not ready for linux");
			return FAILED;
#endif
			break;
		default:
			output_error("instance_slave_get_data(): unrecognized global_multirun_connection state");
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
	STATUS rv;
	MESSAGE msg;

	rv = instance_slave_get_data(&msg, 0, sizeof(MESSAGE));
	if(rv == FAILED){
		output_error("instance_slave_get_header(): get_data call failed");
		return FAILED;
	}
	if(msg.name_size < 0){
		output_error("instance_slave_get_header(): negative msg name_size");
		return FAILED;
	}
	if(msg.data_size < 0){
		output_error("instance_slave_get_header(): negative msg data_size");
		return FAILED;
	}
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
		output_error("instance_slave_parse_prop_list(): null line pointer");
		return FAILED;
	}
	if(root == 0){
		output_error("instance_slave_parse_prop_list(): null root pointer");
		return FAILED;
	}

	output_verbose("parser list: \'%s\'", line);

	token = strtok(line, ", \0\n\r");	// @todo should use strtok_r, will change later
	while(token != 0){
		tokenct = sscanf(token, "%[A-Za-z0-9_].%[A-Za-z0-9_]", objname, propname);
		if(2 != tokenct){
			// @todo check for global properties instead
			output_error("instance_slave_link_properties(): unable to parse '%s' (ct = %d)", token, tokenct);
			return FAILED;
		}
		obj = object_find_name(objname);
		if(obj == 0){
			output_error("instance_slave_link_properties(): unable to find object '%s'", objname);
			return FAILED;
		}
		prop = object_get_property(obj, propname,NULL);
		if(prop == 0){
			output_error("instance_slave_link_properties(): prop '%s' not found in object '%s'", propname, objname);
			return FAILED;
		}
		output_verbose("found links for %s", token);
		// build link
		link = (linkage *)malloc(sizeof(linkage));
		if(0 == link){
			output_error("instance_slave_link_properties(): malloc failure for linkage");
			return FAILED;
		}
		memset(link, 0, sizeof(linkage));
		output_verbose("making link for \'%s.%s\'", objname, propname);
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
	char *buffer = 0;
//	char *work, *write;
	char *read = 0;
	OBJECT *obj = 0;
	PROPERTY *prop = 0;
	STATUS rv = 0;
	size_t offset = 0;
	linkage *link = 0;
	size_t maxlen = 0;
	size_t pickle_size = 0;

	output_verbose("instance_slave_link_properties(): entered");
	if(local_inst.message == 0){
		output_error("instance_slave_link_properties(): local_inst message wrapper not initialized");
		return FAILED;
	}
	if(local_inst.name_size > 0){
		buffer = (char *)malloc(local_inst.name_size);
		if(0 == buffer){
			output_error("instance_slave_link_properties(): malloc failure");
			return FAILED;
		}
		memset(buffer, 0, local_inst.name_size);
	} else {
		output_error("instance_slave_link_properties(): non-positive local_inst.name_size");
		return FAILED;
	}

	
	maxlen = (size_t)(local_inst.name_size);
	// copy to preserve the original buffer
	strcpy(buffer, local_inst.message->name_buffer);

	// split the string manually
	read = strchr(buffer, ' ');
	*read++ = 0;

	// parse writers (first half of the original string)
	// a.b,c.d,e.f' '
	rv = instance_slave_parse_prop_list(buffer, &(local_inst.write), LT_MASTERTOSLAVE);
	if(FAILED == rv){
		output_error("instance_slave_link_properties(): writer list parsing failed");
		return FAILED;
	}

	// parse readers (second half of the original string)
	// a.b,c.d,e.f'\0'
	rv = instance_slave_parse_prop_list(read, &(local_inst.read), LT_SLAVETOMASTER);
	if(FAILED == rv){
		output_error("instance_slave_link_properties(): reader list parsing failed");
		return FAILED;
	}

	// cache pickle'd prop size, if available.
	pickle_size = local_inst.prop_size;
	local_inst.prop_size = 0;

	// iterate through writers and readers to determine memory offsets
	for(link = local_inst.write; link != 0; link = link->next){
		local_inst.prop_size += link->prop_size;
		local_inst.name_size += link->name_size;
		++local_inst.writer_count;
	}
	for(link = local_inst.read; link != 0; link = link->next){
		local_inst.prop_size += link->prop_size;
		local_inst.name_size += link->name_size;
		++local_inst.reader_count;
	}
	// data_buffer will be unallocated if we do not know the link sizes a priori
	if(0 == local_inst.message->data_buffer){
		local_inst.message->data_size = (int16 *)&(local_inst.prop_size);
		local_inst.message->data_buffer = (char *)malloc(local_inst.prop_size);
	}
	offset = 0;
	for(link = local_inst.write; link != 0; link = link->next){
		link->addr = local_inst.message->data_buffer + offset;
		offset += link->prop_size;
	}
	for(link = local_inst.read; link != 0; link = link->next){
		link->addr = local_inst.message->data_buffer + offset;
		offset += link->prop_size;
	}
	// compare pickles
	if(0 != pickle_size){
		if(pickle_size != local_inst.prop_size){
			output_warning("instance_slave_link_properties(): pickle prop_size and calculated prop_size do not match!");
		}
	}
	output_debug("instance_slave_link_properties(): exiting for slave %"FMT_INT64, local_inst.cacheid);
	return SUCCESS;
}

int instance_slave_wait_mmap(){
	int status = 0;
	MESSAGE *tc = 0;
#ifdef WIN32
	DWORD rc = 0;
	tc = (MESSAGE *)(local_inst.filemap);
	output_verbose("wait_mmap: started wait with with fmap ts = %lli", tc->ts);
	rc = WaitForSingleObject(local_inst.hSlave,global_signal_timeout);
	switch ( rc ) {
		case WAIT_ABANDONED:
			output_error("instance_slave_wait_mmap(): slave %d wait abandoned", slave_id);
			break;
		case WAIT_OBJECT_0:
			output_verbose("instance_slave_wait_mmap(): slave %d wait completed", slave_id);
			status = 1;
			break;
		case WAIT_TIMEOUT:
			output_error("instance_slave_wait_mmap(): slave %d wait timeout", slave_id);
			break;
		case WAIT_FAILED:
			output_error("instance_slave_wait_mmap(): slave %d wait failed (error code %d)",slave_id,GetLastError());
			break;
		default:
			output_error("instance_slave_wait_mmap(): slave %d wait return code not recognized (rc=%d)", slave_id,rc);
			break;
	}
	ResetEvent(local_inst.hSlave);
	// copy the data from the mmap to the cache
	memcpy(local_inst.cache, local_inst.filemap, local_inst.cachesize);
	printcontent(local_inst.cache, local_inst.cachesize);
	output_verbose("wait_mmap: resumed with fmap ts = %lli", tc->ts);
#else
	// fatal error
#endif

	
	/* copy inbound linkages */
//	output_verbose("instance_slave_wait_mmap(): slave %d controller reading links", slave_id);
	memcpy(local_inst.cache, local_inst.filemap, local_inst.cachesize);

	return status;
}

int instance_slave_wait_socket(){
	int status = 0;
	int rv = 0;
	size_t offset = 0;

	output_debug("instance_slave_wait_socket(): entered");
	if(0 == local_inst.buffer){
//		output_verbose("instance_slave_wait_mmap(): local_inst.buffer unallocated");
		local_inst.buffer_size = 1 + local_inst.prop_size + sizeof(MESSAGE) + strlen(MSG_DATA);
		local_inst.buffer = (char *)malloc(local_inst.buffer_size);
		if(0 == local_inst.buffer){
			output_fatal("instance_slave_wait_socket(): malloc failure");
			return 0;
		}
	}

	// read socket
	rv = recv(local_inst.sockfd, local_inst.buffer, (int)local_inst.buffer_size, 0);
	printcontent(local_inst.buffer, rv);
//	output_debug("instance_slave_wait_socket(): %d = recv(%d, %x, %d, 0)", rv, local_inst.sockfd, local_inst.buffer, (int)local_inst.buffer_size);
	if(0 == rv){
		output_error("instance_slave_wait_socket(): socket closed");
		closesocket(local_inst.sockfd);
		return 0;
	} else if(0 > rv){
		output_error("instance_slave_wait_socket(): error %d when recv'ing data", rv);
		closesocket(local_inst.sockfd);
		return 0;
	}
	// copy into cache
	offset = strlen(MSG_DATA);
	if(0 != memcmp(local_inst.buffer, MSG_DATA, offset)){
		output_fatal("instance_slave_wait_socket(): memcmp failure");
		return 0;
	}

//	output_debug("instance_slave_wait_socket(): msg memcpy(%x, %x, %d)", local_inst.cache, local_inst.buffer + offset, sizeof(MESSAGE));
	memcpy(local_inst.cache, local_inst.buffer + offset, sizeof(MESSAGE));
	offset += sizeof(MESSAGE);

//	output_debug("instance_slave_wait_socket(): db memcpy(%x, %x, %d)", local_inst.message->data_buffer, local_inst.buffer + offset, local_inst.prop_size);
	memcpy(local_inst.message->data_buffer, local_inst.buffer + offset, local_inst.prop_size);
	status = 1;
	output_debug("instance_slave_wait_socket(): exiting");
	return status;
}

/** instance_slave_wait
	Place slave in wait state until master signal it to resume
	@return 1 on success, 0 on failure.
 **/
int instance_slave_wait(void)
{
	int status = 0;
	output_verbose("instance_slave_wait(): slave %d entering wait state with t2=%lli (%x)", slave_id, local_inst.cache->ts, ((char*)&local_inst.cache->ts-(char*)local_inst.cache));
	if(local_inst.cnxtype == CI_MMAP){
#ifdef WIN32
		status = instance_slave_wait_mmap();
#else
		// @todo linux/unix slave signalling
#endif
	} else if(local_inst.cnxtype == CI_SOCKET){
		status = instance_slave_wait_socket();
	} else if(local_inst.cnxtype == CI_SHMEM){
		; // linux-specific
	}
	/* signal main loop to resume with new timestamp */
	return status;
}

int instance_slave_done_mmap(){
	// this works for MMAP, needs function with switches (or struct w/ func* )
	output_verbose("instance_slave_done_mmap(): copying %d bytes from %x to %x", local_inst.cachesize, local_inst.cache, local_inst.filemap);
	memcpy(local_inst.filemap, local_inst.cache, local_inst.cachesize);
//	printcontent(local_inst.filemap, (int)local_inst.cachesize);

#ifdef WIN32
	SetEvent(local_inst.hMaster);
#else
	// @todo linux/unix slave signalling
#endif
	return 0;
}

int instance_slave_done_socket(){
	size_t offset = 0;
	int rv = 0;
	
//	output_debug("instance_slave_done_socket(): enter");
	if(0 == local_inst.buffer){
		local_inst.buffer_size = 1 + local_inst.prop_size + sizeof(MESSAGE) + strlen(MSG_DATA);
		local_inst.buffer = (char *)malloc(local_inst.buffer_size);
		if(0 == local_inst.buffer){
			output_fatal("instance_slave_done_socket(): malloc failure");
			return -1;
		}
	}
	offset = strlen(MSG_DATA);
	memset(local_inst.buffer, 0, local_inst.buffer_size);
	strcpy(local_inst.buffer, MSG_DATA);
	memcpy(local_inst.buffer + offset, local_inst.cache, sizeof(MESSAGE));
	offset += sizeof(MESSAGE);
	memcpy(local_inst.buffer + offset, local_inst.message->data_buffer, local_inst.prop_size);
	offset += local_inst.prop_size;

	// feed through socket
	rv = send(local_inst.sockfd, local_inst.buffer, (int)offset, 0);
	printcontent(local_inst.buffer, rv);
//	output_debug("instance_slave_done_socket(): %d = send(%d, %x, %d, 0)", rv, local_inst.sockfd, local_inst.buffer, offset);
	if(rv < 0){
		output_error("instance_slave_done_socket(): error when sending data");
		closesocket(local_inst.sockfd);
		// TODO this should be a valid exitcode (see exec.c XC_*)
		return -1;
	} else if(0 == rv){
		output_error("instance_slave_done_socket(): socket was closed before data sent");
		closesocket(local_inst.sockfd);
		// TODO this should be a valid exitcode (see exec.c XC_*)
		return -1;
	}
//	output_debug("instance_slave_done_socket(): exiting");
	return 0;
}

/** instance_slave_done
	Signal the master that the slave is done.
 **/
void instance_slave_done(void)
{
	int rv = 0;
	// write cache to cnx medium

	// @todo cnx exchange
	/* signal the master */
	output_verbose("instance_slave_done(): slave %d signaling master", slave_id);
	switch(local_inst.cnxtype){
		case CI_MMAP:
			rv = instance_slave_done_mmap();
			break;
		case CI_SHMEM:
			break;
		case CI_SOCKET:
			rv = instance_slave_done_socket();
			break;
		default:
			output_error("instance_slave_done(): unrecognized connection type");
			return;
	}
	// check rv, 0 is okay
	if(0 != rv){
		output_error("instance_slave_done(): unable to signal master");
		exec_setexitcode(rv);
	}
}

/** instance_slaveproc
    Create main slave control loop to maintain sync with master.
	Anything that is dependant on objects being loaded happens here.
 **/
void *instance_slaveproc(void *ptr)
{
	linkage *lnk;
	STATUS rv = SUCCESS;
	output_verbose("instance_slaveproc(): slave %d controller startup in progress", slave_id);

	pthread_mutex_lock(&mls_inst_lock);
	pthread_cond_wait(&mls_inst_signal, &mls_inst_lock);
	pthread_mutex_unlock(&mls_inst_lock);

	rv = instance_slave_link_properties();

	instance_slave_done(); // signals to master that this side's ready

	do{
		/* wait for master to signal slave */
		output_verbose("instance_slaveproc(): slave %d controller waiting", slave_id);
		if ( 0 == instance_slave_wait() )
		{
			/* stop the main loop and exit the slave controller */
			output_error("instance_slaveproc(): slave %d controller wait failure, thread stopping", slave_id);
			exec_setexitcode(XC_PRCERR);
			pthread_cond_broadcast(&mls_inst_signal);
			break;
		}

		output_debug("instance_slaveproc(): slave %d controller reading links", slave_id);
		for ( lnk=local_inst.write ; lnk!=NULL ; lnk=lnk->next ){
			if(FAILED == linkage_master_to_slave(0, lnk)){
				output_warning("instance_slaveproc(): linkage_master_to_slave failed");
				rv = FAILED;
				break;
			}
		}

//		output_debug("continuing");
		
		/* resume the main loop */
		// note, if TS_NEVER, we want the slave's exec loop to end normally
		//output_debug("slave %d controller resuming exec with %lli", slave_id, local_inst.cache->ts);
		output_debug("slave %d controller resuming exec with %lli", local_inst.cache->id, local_inst.cache->ts);
		output_debug("slave %d controller setting step_to %lli to cache->ts %lli", local_inst.cache->id, exec_sync_get(NULL), local_inst.cache->ts);
		exec_sync_merge(NULL,&local_inst.cache);

		pthread_cond_broadcast(&mls_inst_signal);

		if(local_inst.cache->ts == TS_NEVER){
			break;
		}

		/* wait for main loop to pause */
		output_verbose("slave %d controller waiting for main to complete", slave_id);

		pthread_mutex_lock(&mls_inst_lock);
		pthread_cond_wait(&mls_inst_signal, &mls_inst_lock);
		pthread_mutex_unlock(&mls_inst_lock);

		/* @todo copy output linkages */
		output_debug("slave %d controller writing links", slave_id);
		for ( lnk=local_inst.read ; lnk!=NULL ; lnk=lnk->next ){
			if(FAILED == linkage_slave_to_master(0, lnk)){
				output_warning("linkage_slave_to_master failed");
				rv = FAILED;
				break;
			}
		}
//		output_debug("continuing");

		/* copy the next time stamp */
		/* how about we copy the time we want to step to and see what the master says, instead? -MH */
		exec_sync_reset(&local_inst.cache);
		exec_sync_set(&local_inst.cache,NULL);

		instance_slave_done();
	} while (global_clock != TS_NEVER && rv == SUCCESS);
	output_verbose("slave %"FMT_INT64" completion state reached", local_inst.cacheid);
	pthread_exit(NULL);
	return NULL;
}



/** do docx here
 **/
STATUS instance_slave_init_mem(){
#ifdef WIN32
	MESSAGE tmsg;
	char eventName[256];
	char cacheName[256];
	int64 rv = 0;
	size_t sz = sizeof(MESSAGE);
	size_t cacheSize;

	output_debug("instance_slave_init_mem()");
	/* @todo open cache */
	local_inst.cacheid = global_master_port;
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
	rv = (int64)local_inst.filemap;

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
	local_inst.buffer_size = local_inst.cachesize = cacheSize = tmsg.asize;
	local_inst.buffer = (char *)malloc(local_inst.cachesize);
	local_inst.cache = (MESSAGE *)malloc(local_inst.cachesize);
	local_inst.id = slave_id = tmsg.id;

	// THIS COPIES THE DATA
	memcpy(local_inst.cache, local_inst.filemap, local_inst.cachesize);
	messagewrapper_init(&(local_inst.message), local_inst.cache);
	
	local_inst.name_size = *(local_inst.message->name_size);
	local_inst.prop_size = *(local_inst.message->data_size);
	exec_sync_merge(NULL,&local_inst.cache);

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

/**	must handshake by connecting and sending 'just' a message
 **	 struct.  proper response is a message struct with the same ID
 **	 and nonzero sizes.  ack with same struct, after malloc'ing
 **	 buffer space.  response after should contain names.  responses
 **	 after that will be data messages.
 **/
STATUS instance_slave_init_socket(){
	int rv = 0;
	struct sockaddr_in connaddr;
	char cmd[1024];
	FILE *refile = 0;
	INSTANCE_PICKLE pickle;
#ifdef WIN32
	static WSADATA wsaData;
#endif

	// start up WinSock, if on Windows
#ifdef WIN32
	output_debug("starting WS2");
	if (WSAStartup(MAKEWORD(2,0),&wsaData)!=0)
	{
		output_error("instance_cnx_socket(): socket library initialization failed: %s",strerror(GetLastError()));
		return FAILED;
	}
#endif

	// connect to master
	local_inst.sockfd = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(INVALID_SOCKET == local_inst.sockfd){
		output_fatal("instance_slave_init_socket(): unable to open socket");
		return FAILED;
	}
	memset(&connaddr, 0, sizeof(connaddr));
	connaddr.sin_addr.s_addr = inet_addr(global_master);
	if(INADDR_NONE == connaddr.sin_addr.s_addr){
		output_fatal("instance_slave_init_socket(): unrecognized inet_addr \'%s\'", global_master);
		return FAILED;
	}
	connaddr.sin_port = htons((unsigned short)global_master_port);
	connaddr.sin_family = AF_INET;
	rv = connect(local_inst.sockfd, (struct sockaddr *)&connaddr, sizeof(connaddr));
	if(0 != rv){
		output_fatal("instance_slave_init_socket(): could not connect to %s:%d", global_master, global_master_port);
		#ifdef WIN32
			output_error("WSA error: %d", WSAGetLastError());
		#else
			perror("getsockname():");
		#endif
		return FAILED;
	}
	// handshake?
	sprintf(cmd, HS_CBK "%"FMT_INT64"d", global_slave_id);
	output_debug("cmd/cbk: %s", cmd);
	rv = send(local_inst.sockfd, cmd, (int)strlen(cmd), 0);
	if(0 == rv){
		output_fatal("instance_slave_init_socket(): socket closed before slave handshake sent");
		return FAILED;
	} else if(0 > rv){
		output_fatal("instance_slave_init_socket(): error sending slave handshake");
		return FAILED;
	}
	// get response
	rv = recv(local_inst.sockfd, cmd, 1024, 0);
	if(rv == 0){
		output_fatal("instance_slave_init_socket(): socket closed before slave handshake response recv'd");
		return FAILED;
	} else if (0 > rv){
		output_fatal("instance_slave_init_socket(): error receiving slave handshake response");
		return FAILED;
	}
	if(0 == memcmp(cmd, HS_FAIL, strlen(HS_FAIL))){
		output_fatal("instance_slave_init_socket(): master reports bad ID/handshake");
		return FAILED;
	}
	if(0 != memcmp(cmd, HS_RSP, strlen(HS_RSP))){
		output_fatal("instance_slave_init_socket(): unrecognized master handshake reply response");
		return FAILED;
	}

	// recv instance struct
	rv = recv(local_inst.sockfd, cmd, 1024, 0);
	if(rv == 0){
		output_fatal("instance_slave_init_socket(): socket closed before instance data recv'd");
		return FAILED;
	} else if (0 > rv){
		output_fatal("instance_slave_init_socket(): error receiving instance data");
		return FAILED;
	}
	if(0 != memcmp(cmd, MSG_INST, strlen(MSG_INST))){
		;
	}
	memcpy(&pickle, cmd+strlen(MSG_INST), sizeof(pickle));
	output_debug("pickle: %"FMT_INT64"d %d %d %d %d %"FMT_INT64"d", pickle.cacheid, pickle.cachesize, pickle.name_size, pickle.prop_size, pickle.id, pickle.ts);
	if(local_inst.cacheid != pickle.cacheid){
		; // error
	}
	local_inst.buffer_size = local_inst.cachesize = pickle.cachesize;
	local_inst.name_size = pickle.name_size;
	local_inst.prop_size = pickle.prop_size;
	local_inst.id = slave_id = pickle.id;
	local_inst.buffer = (char *)malloc(local_inst.cachesize);
	local_inst.cache = (MESSAGE *)malloc(local_inst.cachesize);
	memset(local_inst.buffer, 0, local_inst.cachesize);
	memset(local_inst.cache, 0, local_inst.cachesize);
	local_inst.cache->name_size = (int16)local_inst.name_size;
	local_inst.cache->data_size = (int16)local_inst.prop_size;
	local_inst.cache->id = local_inst.id;
	exec_sync_set(&local_inst.cache,pickle.ts);
	exec_sync_merge(NULL,&local_inst.cache);
	if(0 == local_inst.buffer){
		output_error("malloc() error with li.buffer");
		return FAILED;
	}
	if(0 == local_inst.cache){
		output_error("malloc() error with li.cache");
		return FAILED;
	}
	messagewrapper_init(&(local_inst.message), local_inst.cache);
	output_debug("mw: m %x, n %x (%d), d %x (%d)", local_inst.message->msg, local_inst.message->name_buffer, *(local_inst.message->name_size), local_inst.message->data_buffer, *(local_inst.message->data_size));
	local_inst.message->name_size = (int16 *)&local_inst.name_size;
	local_inst.message->data_size = (int16 *)&local_inst.prop_size;
	// receipt instance struct
	// receipt linkage data
	strcpy(cmd, MSG_OK);
	rv = send(local_inst.sockfd, cmd, (int)strlen(MSG_OK), 0);
	if(0 == rv){
		output_fatal("instance_slave_init_socket(): socket closed before instance OK sent");
		return FAILED;
	} else if(0 > rv){
		output_fatal("instance_slave_init_socket(): error sending instance OK");
		return FAILED;
	}

	// recv linkage information
	output_debug("recv'ing linkage data");
	rv = recv(local_inst.sockfd, cmd, 1024, 0);
	if(rv == 0){
		output_fatal("instance_slave_init_socket(): socket closed before linkage names recv'd");
		return FAILED;
	} else if (0 > rv){
		output_fatal("instance_slave_init_socket(): error receiving linkage names ");
		return FAILED;
	}
	if(0 != memcmp(cmd, MSG_LINK, strlen(MSG_LINK))){
		output_fatal("instance_slave_init_socket(): master did not send MSG_LINK message when expected");
		return FAILED;
	}
	memcpy(local_inst.message->name_buffer, cmd+strlen(MSG_LINK), local_inst.name_size);
	
	
	// receipt linkage data
	strcpy(cmd, MSG_OK);
	rv = send(local_inst.sockfd, cmd, (int)strlen(MSG_OK), 0);
	if(0 == rv){
		output_fatal("instance_slave_init_socket(): socket closed before link name OK sent");
		return FAILED;
	} else if(0 > rv){
		output_fatal("instance_slave_init_socket(): error sending link name OK");
		return FAILED;
	}

	// receive start message
	rv = recv(local_inst.sockfd, cmd, (int)strlen(MSG_START), 0);
	if(rv == 0){
		output_fatal("instance_slave_init_socket(): socket closed before start message recv'd");
		return FAILED;
	} else if (0 > rv){
		output_fatal("instance_slave_init_socket(): error receiving start message");
		return FAILED;
	}
	if(0 != memcmp(cmd, MSG_START, (int)strlen(MSG_START))){
		output_fatal("instance_slave_init_socket(): master did not send MSG_START message when expected");
		output_debug(cmd);
		return FAILED;
	} else {
		output_debug("instance_slave_init_socket(): recv start message");
	}

	// send initial link data 

	/* NOTE: may be feeding stdio down the socket to the master after this point */

	// fdopen master socket
/*	local_inst.stream = fdopen(local_inst.sockfd, "w+");
	if(0 == local_inst.stream){
		output_fatal("instance_slave_init_socket(): unable to fdopen() the socket");
		return FAILED;
	}
	output_redirect_stream("output", local_inst.stream);
	output_redirect_stream("error", local_inst.stream);
	output_redirect_stream("warning", local_inst.stream);
	output_redirect_stream("debug", local_inst.stream);
	output_redirect_stream("verbose", local_inst.stream);
	output_redirect_stream("profile", local_inst.stream);
	output_redirect_stream("progress", local_inst.stream);*/

	output_debug("end of instance_slave_init_socket()");
	return SUCCESS;
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
	// !!! &local_inst.pid causes a warning, pthread_t * -> us*__w64, but pt_t is a struct with [void * + uint], bad recast
	if ( pthread_create(&(local_inst.threadid), NULL, instance_slaveproc, NULL) )
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
//	STATUS stat;
	size_t cacheSize = sizeof(MESSAGE)+MSGALLOCSZ; /* @todo size instance cache dynamically from linkage list */

	output_debug("instance_slave_init()");

//	memset(&local_inst, 0, sizeof(local_inst));
	local_inst.cacheid = global_slave_id;
	global_multirun_mode = MRM_SLAVE;
	output_debug("slave %lli setting prefixes", local_inst.cacheid);
	output_prefix_enable();
	

	output_debug("slave %lli initializing connection", local_inst.cacheid);
	// this is where we open a connection, snag the first header, and initialize everything from there
	switch(global_multirun_connection){
		case MRC_MEM:
#ifdef WIN32
			local_inst.cnxtype = CI_MMAP;
#else
			local_inst.cnxtype = CI_SHMEM;
#endif
			rv = instance_slave_init_mem();
			break;
		case MRC_SOCKET:
			local_inst.cnxtype = CI_SOCKET;
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

	output_debug("li: %llx %d %d %d %d", local_inst.cacheid, local_inst.cachesize, local_inst.name_size, local_inst.prop_size, local_inst.id);
//	output_debug("slave %"FMT_INT64" entering init_pthreads()", local_inst.cacheid);
	rv = instance_slave_init_pthreads(); // starts slaveproc() thread
//	output_debug("slave %"FMT_INT64" exited init_pthreads()", local_inst.cacheid);
//	output_debug("li: %"FMT_INT64" %d %d %d %d", local_inst.cacheid, local_inst.cachesize, local_inst.name_size, local_inst.prop_size, local_inst.id);
	
	global_clock = exec_sync_get(NULL); // copy time signal to gc, legit since it's from msg
	output_debug("inst_slave_init(): gc = %lli", global_clock);

	// signal master that slave init is done
//	instance_slave_done();
	// punt this until after first inst_signal sent

	return SUCCESS;
}


// EOF
