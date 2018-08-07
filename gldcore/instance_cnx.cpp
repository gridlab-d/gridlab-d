#include "instance_cnx.h"

//extern pthread_mutex_t inst_sock_lock;
extern pthread_cond_t inst_sock_signal;
extern int sock_created;

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
	char cmd[1024];
	char sendcmd[1024];
	INSTANCE_PICKLE pickle;
	char *colon;
	int rv, return_addr_sz;
	unsigned int64 check_id;
	size_t bytes_to_send, msg_link_sz, idx;
//	size_t sent_bytes;
	SOCKET outsockfd, insockfd;
	struct sockaddr_in outaddr, return_addr;
//	struct fd_set master, listener;
//	FILE *file_to_send;
	fd_set callback_fdset;
	struct timeval timer;
	struct sockaddr_storage ss;
	int slt;
	char blank[8];
	char *args[7] = {"--profile", "--relax", "--debug", "--verbose",
		"--warn", "--quiet", "--avlbalance"};
#ifdef WIN32
	static WSADATA wsaData;
#endif
	char rsp[32];
	strcpy(rsp, HS_FAIL);

	strcpy(blank, "");

	// prepare intermediate buffer
	inst->buffer = (char *)malloc(inst->cachesize);
	if(0 == inst->buffer){
		return FAILED;
	} else {
		memset(inst->buffer, 0, inst->cachesize);
	}

	// un-mangle hostname
	colon = strchr(inst->hostname, ':');
	if(colon != 0){
		inst->port = (unsigned short)strtol(colon+1, NULL, 10);
		*colon = 0;
	}
	// create socket
#ifdef WIN32
	output_debug("starting WS2");
	if (WSAStartup(MAKEWORD(2,0),&wsaData)!=0)
	{
		output_error("instance_cnx_socket(): socket library initialization failed: %s",strerror(GetLastError()));
		return FAILED;
	}
#endif
	outsockfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(INVALID_SOCKET == outsockfd){
		output_error("instance_cnx_socket(): unable to open outbound socket");
		return FAILED;
	}
	// connect to slavenode on remote machine
	memset(&outaddr, 0, sizeof(outaddr));
	outaddr.sin_family = AF_INET;
	outaddr.sin_addr.s_addr = inet_addr(inst->hostname);
	if(INADDR_NONE == outaddr.sin_addr.s_addr){
		output_error("instance_cnx_socket(): unable to translate hostname '%s'", inst->hostname);
		return FAILED;
	}
	outaddr.sin_port = htons(inst->port);
	rv = connect(outsockfd, (const struct sockaddr *)&outaddr, sizeof(outaddr));
	if(0 != rv){
		output_error("instance_cnx_socket(): unable to connect to slave at %s:%i", inst->hostname, inst->port);
		return FAILED;
	}

	// send HS_SYN
	sprintf(sendcmd, HS_SYN);
	rv = send(outsockfd, sendcmd, 1+(int)strlen(HS_SYN), 0);
	output_debug("%d = send(%d, %x, %d, 0)", rv, outsockfd, sendcmd, 1+strlen(HS_SYN));
	if(1 > rv){
		output_error("instance_cnx_socket(): error sending handshake");
		return FAILED;
	}
	// recv HS_ACK
	rv = (int)recv(outsockfd, sendcmd, 1024, 0);
	output_debug("%d = recv(%d, %x, 1024, 0)", rv, outsockfd, sendcmd);
	if(0 > rv){
		output_error("instance_cnx_socket(): error receiving handshake response");
		return FAILED;
	}
	if(0 == rv){
		output_error("instance_cnx_socket(): socket closed before receiving handshake response");
		return FAILED;
	}
	if(0 != memcmp(sendcmd, HS_ACK, strlen(HS_ACK))){
		output_error("instance_cnx_socket(): handshake response mismatch");
		return FAILED;
	}

	// socket & bind callback port; if port = 0, OS will randomly assign an open port
	// open listener socket
	insockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(-1 == insockfd){
		output_error("instance_cnx_socket(): error when constructing callback socket");
		return FAILED;
	}
	// bind listener socket
	memset(&return_addr, 0, sizeof(return_addr));
	return_addr.sin_family = AF_INET;
	return_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	return_addr.sin_port = htons(inst->return_port);
	if(0 != bind(insockfd, (struct sockaddr *)&return_addr, sizeof(return_addr))){
		output_error("instance_cnx_socket(): error when binding callback socket");
		output_error("fd %d, addr %x, port %i", insockfd, return_addr.sin_addr.s_addr, inst->return_port);
		perror("bind():");
		closesocket(insockfd);
		return FAILED;
	}
	// get listener port
	slt = sizeof(ss);
	if(-1 == getsockname(insockfd, (struct sockaddr *)&ss, &slt)){
		output_error("instance_cnx_socket(): error getting insockfd name");
#ifdef WIN32
		WSAGetLastError();
#else
		perror("getsockname():");
#endif
		closesocket(insockfd);
		return FAILED;
	}
	
	// should be listening BEFORE we tell the slave to call back...
	if(0 != listen(insockfd, 2)){
		output_error("instance_cnx_socket(): unable to listen on callback socket");
		closesocket(insockfd);
		return FAILED;
	}
	
	inst->return_port = ntohs(((struct sockaddr_in *)(&ss))->sin_port);
	output_debug("instance_cnx_socket(): callback listening on port %d", inst->return_port);

	// build command
	// HS_CMD dir file r_port cacheid profile relax debug verbose warn quiet avlbalance
	sprintf(sendcmd, HS_CMD	"dir=\"%s\" file=\"%s\" port=%d id=%"FMT_INT64"d %s %s %s %s %s %s %s",
		inst->execdir,
		inst->model,
		inst->return_port,
		inst->cacheid,
		global_profiler ? args[0] : blank,
		global_strictnames ? blank : args[1],
		global_debug_output ? args[2] : blank,
		global_verbose_mode ? args[3] : blank,
		global_warn_mode ? blank : args[4],
		global_quiet_mode ? args[5] : blank,
		global_no_balance ? args[6] : blank);
	output_debug("cmdstr(%d): %s", rv, sendcmd);

	// send command
	// "execdir/bin/gridlabd slaveX.glm --verbose --debug --slave my.hostname.gov:6767"
	// * probably could skip the host, extract that from the sockaddr on the far side
	// ** inst->model contains the file name
	// ** input files not currently recognized
	rv = send(outsockfd, sendcmd, 1+(int)strlen(sendcmd), 0);
	output_debug("%d = send(%d, %x, %d, 0)", rv, outsockfd, sendcmd, 1+strlen(sendcmd));
	if(1 > rv){
		output_error("instance_cnx_socket(): error sending command");
		closesocket(outsockfd);
		return FAILED;
	}
	
	// ! DC says "use a shared filesystem"
	// send file(s)
	// * identify files to send (will require loader changes)
	// * package files together (at least are sending GLM)
	// * loop bite-sized pieces of package
	bytes_to_send = 0;


	// recv receipt of file xfer
	// * come up with receipt message

	// close socket
	rv = closesocket(outsockfd);
	if(rv != 0){
		output_error("instance_cnx_socket(): error when disconnecting from slave");
		return FAILED;
	}

	return_addr_sz = sizeof(return_addr);
	timer.tv_sec = global_signal_timeout/1000;
	timer.tv_usec = global_signal_timeout%1000;

	output_debug("timer: %d.%d", timer.tv_sec, timer.tv_usec);

	FD_ZERO(&callback_fdset);
	FD_SET(insockfd, &callback_fdset);
	rv = select(1+(int)insockfd, &callback_fdset, 0, 0, &timer);
	if(0 == rv){
		// timeout
		output_error("instance_cnx_socket(): callback socket accept() timed out");
		closesocket(insockfd);
		return FAILED;
	} else if(0 > rv){ // -1 == rv
		// error
		output_error("instance_cnx_socket(): callback socket select() error");
		closesocket(insockfd);
		return FAILED;
	}
	
	inst->sockfd = (int)accept(insockfd, (struct sockaddr *)&return_addr, &return_addr_sz);
	if(-1 == inst->sockfd){
		// error
		output_error("instance_cnx_socket(): error accept()ing connection on callback socket");
		closesocket(insockfd);
		return FAILED;
	}

	// handshake
	FD_ZERO(&callback_fdset);
	FD_SET(inst->sockfd, &callback_fdset);
	timer.tv_sec = global_signal_timeout/1000;
	timer.tv_usec = global_signal_timeout%1000;

	// wait for call-back handshake
	rv = select(inst->sockfd+1, &callback_fdset, 0, 0, &timer);
	if(0 == rv){
		output_error("instance_cnx_socket(): select() callback timeout");
		closesocket(inst->sockfd);
		return FAILED;
	} else if(-1 == rv){
		output_error("instance_cnx_socket(): select() callback handshake error");
		closesocket(inst->sockfd);
		return FAILED;
	}

	// receive call-back handshake
	rv = recv(inst->sockfd, cmd, 1024, 0);
	output_debug("%d = recv(%d, %x, 1024, 0)", rv, inst->sockfd, cmd);
	if(rv < 0){
		// error
		output_error("instance_cnx_socket(): callback handshake recv() error");
		closesocket(inst->sockfd);
		return FAILED;
	} else if(0 == rv){
		// closed/timed out (??)
		output_error("instance_cnx_socket(): callback handshake recv() closed");
		closesocket(inst->sockfd);
		return FAILED;
	}
	closesocket(insockfd);

	// compare response to HS_CBK+inst->cacheid
	if(0 != memcmp(cmd, HS_CBK, strlen(HS_CBK))){
		output_error("instance_cnx_socket(): callback handshake mismatch");
		send(inst->sockfd, rsp, (int)strlen(rsp), 0);
		closesocket(inst->sockfd);
		return FAILED;
	}

	// verify ID
	check_id = strtoll(cmd+strlen(HS_CBK), 0, 10);
	if(0 == check_id){
		output_error("instance_cnx_socket(): callback id read error");
		send(inst->sockfd, rsp, (int)strlen(rsp), 0);
		closesocket(inst->sockfd);
		return FAILED;
	}
	if(check_id != inst->cacheid){
		output_error("instance_cnx_socket(): callback id mismatch");
		output_debug(" local: "FMT_INT64"d", inst->cacheid);
		output_debug(" input: "FMT_INT64"d", check_id);
		send(inst->sockfd, rsp, (int)strlen(rsp), 0);
		closesocket(inst->sockfd);
		return FAILED;
	}
	
	// respond to handshake with HS_RSP
	strcpy(cmd, HS_RSP);
	rv = send(inst->sockfd, cmd, (int)strlen(cmd), 0);
	output_debug("%d = send(%d, %x, %d, 0)", rv, inst->sockfd, cmd, strlen(cmd));
	if(0 == rv){
		output_error("instance_cnx_socket(): socket closed before slave handshake response send");
		closesocket(inst->sockfd);
		return FAILED;
	} else if(0 > rv){
		output_error("instance_cnx_socket(): error sending slave handshake response");
		closesocket(inst->sockfd);
		return FAILED;
	}

	// fill instance struct
	pickle.cacheid = inst->cacheid;
	pickle.cachesize = (int16)(inst->cachesize);
	pickle.name_size = (int16)(inst->name_size);
	pickle.prop_size = (int16)(inst->prop_size);
	pickle.id = inst->id;
	pickle.ts = global_clock;
	output_debug("pickle: %"FMT_INT64"d %d %d %d %d %"FMT_INT64, pickle.cacheid, pickle.cachesize, pickle.name_size, pickle.prop_size, pickle.id, pickle.ts);
	// send instance struct
	memset(cmd, 0, sizeof(cmd));
	memcpy(cmd, MSG_INST, strlen(MSG_INST));
	memcpy(cmd+strlen(MSG_INST), &pickle, sizeof(pickle));
	rv = send(inst->sockfd, cmd, (int)(strlen(MSG_INST)+sizeof(pickle)), 0);
	output_debug("rv = send(%d, %x, %d, 0)", rv, inst->sockfd, cmd, strlen(MSG_INST)+sizeof(pickle));
	if(0 == rv){
		output_error("instance_cnx_socket(): socket closed before slave handshake response send");
		closesocket(inst->sockfd);
		return FAILED;
	} else if(0 > rv){
		output_error("instance_cnx_socket(): error sending slave handshake response");
		closesocket(inst->sockfd);
		return FAILED;
	}
	// recv receipt of instance struct
	rv = recv(inst->sockfd, cmd, 1024, 0);
	output_debug("%d = recv(%d, %x, 1024, 0)", rv, inst->sockfd, cmd);
	if(0 == rv){
		output_error("instance_cnx_socket(): socket closed before recving instance receipt");
		return FAILED;
	} else if(0 > rv){
		output_error("instance_cnx_socket(): error recving instance receipt");
		return FAILED;
	}
	if(0 != memcmp(cmd, MSG_OK, strlen(MSG_OK))){
		output_error("instance_cnx_socket(): instance receipt mismatch");
		return FAILED;
	}

	// send linkage data
	//	remember, inst->msg->name_size is an int16 *
	msg_link_sz = strlen(MSG_LINK);
	memcpy(sendcmd, MSG_LINK, msg_link_sz);
	memcpy(sendcmd+msg_link_sz, inst->message->name_buffer, *(inst->message->name_size));
	idx = msg_link_sz + *(inst->message->name_size);
	sendcmd[idx] = 0;

	rv = send(inst->sockfd, sendcmd, (int)msg_link_sz + *(inst->message->name_size) + 1, 0);
	output_debug("%d = send(%d, %x, %d, 0)", rv, inst->sockfd, sendcmd, (int)msg_link_sz + *(inst->message->name_size) + 1);
	if(rv == -1){
		output_error("instance_cnx_socket(): error sending linkage data");
		return FAILED;
	} else if(rv == 0){
		output_error("instance_cnx_socket(): socket closed before linkage data sent");
		return FAILED;
	}

	// recv linkage receipt
	rv = recv(inst->sockfd, cmd, 1024, 0);
	output_debug("%d = recv(%d, %x, 1024, 0)", rv, inst->sockfd, cmd);
	if(0 == rv){
		output_error("instance_cnx_socket(): socket closed before recving linkage receipt");
		return FAILED;
	} else if(0 > rv){
		output_error("instance_cnx_socket(): error recving linkage receipt");
		return FAILED;
	} else {
		output_debug("i_cnx_s(): recv'ed %d from recv(%d, %x, 1024, 0)", rv, inst->sockfd, cmd);
	}

	sprintf(cmd, MSG_START);
	rv = send(inst->sockfd, cmd, (int)strlen(MSG_START), 0);
	if(0 > rv){
		output_error("instance_cnx_socket(): error sending start message");
		return FAILED;
	} else if(0 == rv){
		output_error("instance_cnx_socket(): socket closed before start message sent");
		return FAILED;
	} else {
		output_debug("instance_cnx_socket(): sent start message");
	}
	// recv initial linkage data
	//	copy into buffer, deal with later
	//	*** can't get this until a bit later
	//  done at done()?

	/* NOTICE: any data recv'ed from inst->sockfd may actually be stdio from the slave */

	// init socket lock & condition
	rv = pthread_mutex_init(&inst->sock_lock,NULL);
	if(rv != 0){
		output_error("error with pthread_mutex_init() in instance_cnx_socket()");
		return FAILED;
	}
	rv = pthread_cond_init(&inst->sock_signal,NULL);
	if(rv != 0){
		output_error("error with pthread_cond_init() in instance_cnx_socket()");
		return FAILED;
	}
	sock_created = 1;

	rv = pthread_mutex_init(&inst->wait_lock,NULL);
	if(rv != 0){
		output_error("error with pthread_mutex_init() in instance_cnx_socket()");
		return FAILED;
	}
	rv = pthread_cond_init(&inst->wait_signal,NULL);
	if(rv != 0){
		output_error("error with pthread_cond_init() in instance_cnx_socket()");
		return FAILED;
	}

	output_debug("end of instance_cnx_socket()");
	return SUCCESS;
}

STATUS instance_connect(instance *inst){
	switch(inst->cnxtype){
		case CI_MMAP:
			if(FAILED == instance_cnx_mmap(inst)){
				output_error("unable to create memory map for instance %s.%s (%llu)", inst->hostname, inst->model, inst->cacheid);
				return FAILED;
			}
			break;
		case CI_SHMEM:
			if(FAILED == instance_cnx_shmem(inst)){
				output_error("unable to create shared memory partition for instance %s.%s", inst->hostname, inst->model);
				return FAILED;
			}

			break;
		case CI_SOCKET:
			if(FAILED == instance_cnx_socket(inst)){
				output_error("unable to create socket connection for instance %s/%s (%llu)", inst->hostname, inst->model, inst->cacheid);
				return FAILED;
			}
			break;
		default:
			output_error("instance %s.%s contains invalid connection type", inst->hostname, inst->model);
			return FAILED;
			break;
	}
	return SUCCESS;
}

// EOF
