// daemon.cpp

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include "gridlabd.h"
#include "output.h"
#include "cmdarg.h"
#include "daemon.h"
#include "globals.h"

static int daemon_pid = 0;
static char clientmask[32] = "0.0.0.0";
static char port[8] = "6266";
static char maxbacklog[8] = "4";
static char logfile[1024] = "gridlabd-log";
static char pidfile[1024] = "gridlabd-pid";
static char workdir[1024] = "/";
static char user[1024] = "gridlabd.gridlabd";

// gridlabd stream specifications
static char output[1024] = "";
static char verbose[1024] = "";
static char warning[1024] = "";
static char debug[1024] = "";
static char profile[1024] = "";
static char progress[1024] = "";
static char error[1024] = "";
static char keepalive[8] = "1";
static char timeout[8] = "10";

static struct s_config {
	char *name;
	char *value;
} config[] = {
	{"log", logfile},
	{"pid", pidfile},
	{"workdir", workdir},
	{"user", user},
	{"output", output},
	{"verbose",verbose},
	{"error", error},
	{"warning",warning},
	{"debug",debug},
	{"profile",profile},
	{"progress",progress},
	{"maxbacklog",maxbacklog},
	{"clientmask",clientmask},
	{"port",port},
	{"keepalive",keepalive},
	{"timeout",timeout},
	NULL, NULL // required to end loop
};

static void daemon_log(char *format, ...)
{
	static int pid = 0;
	char buffer[1024];
	static FILE *logfh = NULL;
	// null format or changed pid (fork) closes the log
	if ( format == NULL || pid != getpid() )
	{
		if ( logfh != NULL )
		{
			fclose(logfh);
			logfh = NULL;
			pid = 0;
		}
	}
	va_list ptr;
	va_start(ptr,format);
	vsprintf(buffer,format,ptr); /* note the lack of check on buffer overrun */
	va_end(ptr);

	// first-time access to log file
	if ( logfh == NULL )
	{
		char *mode = "w";

		// if log file name starts with a +, then use append mode...
		if ( logfile[0] == '+' )
			mode = "a";

		// ...skip the + on the log file name
		logfh = fopen(logfile + (logfile[0]=='+'?1:0), mode);
		pid = getpid();
	}

	// check for failure to access log file
	if ( logfh == NULL )
	{
		output_error("(logfile %s not open) %s",logfile,buffer);
	}

	// make log entry
	else
	{
		time_t t = time(NULL);
		struct tm *tb = localtime(&t);
		char ts[128];
		strftime(ts,sizeof(ts),"%Y-%m-%d %H:%M:%S %z",tb);
		fprintf(logfh,"[%d@%s] %s\n",getpid(),ts,buffer);
		fflush(logfh);
	}
	return;
}

static void daemon_signal(int signum)
{
	switch ( signum ) {
	case SIGHUP:
		daemon_log("SIGHUP received -- terminating daemon"); 
		exit(XC_SIGHUP);
	case SIGINT:
		daemon_log("SIGINT received -- terminating daemon"); 
		exit(XC_SIGINT);
	case SIGKILL:
		daemon_log("SIGKILL received -- terminating daemon"); 
		exit(XC_SIGKILL);
	case SIGTERM:		
		daemon_log("SIGTERM received -- terminating daemon"); 
		exit(XC_SIGTERM);
	}
}

static void daemon_cleanup(void)
{
	if ( daemon_pid == getpid() )
	{
		struct stat fs;

		// remove the pid file
		daemon_log("daemon cleanup");
		if ( unlink(pidfile) == 0 )
			daemon_log("deleted pidfile '%s'",pidfile);

		// only report an error if the file exists
		else if ( errno != 2 )
			daemon_log("unable to delete pidfile '%s': %s",pidfile,strerror(errno));

		// shutdown log
		daemon_log("daemon shutdown");
		daemon_log(NULL); 
	}
}

static bool daemon_savepid(void)
{
	FILE *fp = fopen(pidfile,"w");
	if ( fp == NULL )
		return false;
	if ( fprintf(fp,"%d",getpid()) < 0 )
		return false;
	if ( fclose(fp) != 0 )
		return false;
	return true;
}

static pid_t daemon_readpid(void)
{
	pid_t pid;
	FILE *fp = fopen(pidfile,"r");
	if ( fp == NULL )
		return 0;
	if ( fscanf(fp,"%d",&pid) != 1 )
		return 0;
	if ( fclose(fp) != 0 )
		return 0;
	return pid;
}

// FILE *keepalive_out = NULL;
// int keepalive_time = 0;
// static void daemon_keepalive(int sig)
// {
// 	if ( sig == SIGALRM )
// 	{
// 		fprintf(keepalive_out,".");
// 		fflush(keepalive_out);
// 		alarm(keepalive_time);
// 	}
// }

static int parse_command(char *command, char **argv)
{
#define MAXARGS 256
	int argc = 0;
	argv = (char**)malloc(sizeof(char*)*MAXARGS);
	memset(argv,0,sizeof(char*)*MAXARGS);
	bool in = false;
	char *p;
	for ( p = command ; *p!='\0' && argc < MAXARGS ; p++ )
	{
		if ( in && isspace(*p) )
		{
			*p = '\0';
			in = false;
		}
		else if ( !in && !isspace(*p) )
		{
			argv[argc++] = p;
			in = true;
		}
	}
	if ( argc == MAXARGS && *p != '\0')
	{
		output_error("too many remote command arguments received (only %d parsed)",argc);
	}
	return argc;
}

static void daemon_run_kill(int sig)
{
	if ( sig == SIGALRM )
	{
		daemon_log("server timeout");
		exit(XC_SIGNAL|SIGALRM);
	}
}

static int daemon_run(int sockfd)
{
	FILE *in = fdopen(sockfd,"r");
	FILE *out = fdopen(dup(sockfd),"w");
	int tout = max(0,atoi(timeout));
	signal(SIGALRM,daemon_run_kill);
	alarm(tout);
	char command[1024];
	if ( ! fgets(command,sizeof(command)-1,in) )
	{
		fprintf(out,"ERROR    [INIT] : missing remote command\nexit %d",XC_INIERR);
		fclose(out);
		fclose(in);
		return XC_INIERR;
	}
	else
		output_debug("remote command received: %s",command);
	alarm(0);

	//// implement keepalive mechanism (probably not needed though)
	// keepalive_time = atoi(keepalive);
	// if ( keepalive_time > 0 )
	// {
	// 	keepalive_out = out;
	// 	signal(SIGALRM,daemon_keepalive);
	// 	alarm(keepalive_time);		
	// }	

	// setup simulation output streams
	if ( strcmp(output,"")==0 ) output_redirect_stream("output",out); else output_redirect("output",output);
	if ( strcmp(error,"")==0 ) output_redirect_stream("error",out); else output_redirect("error",error);
	if ( strcmp(warning,"")==0 ) output_redirect_stream("warning",out); else output_redirect("warning",warning);
	if ( strcmp(debug,"")==0 ) output_redirect_stream("debug",out); else output_redirect("debug",debug);
	if ( strcmp(verbose,"")==0 ) output_redirect_stream("verbose",out); else output_redirect("verbose",verbose);
	if ( strcmp(profile,"")==0 ) output_redirect_stream("profile",out); else output_redirect("profile",profile);
	if ( strcmp(progress,"")==0 ) output_redirect_stream("progress",out); else output_redirect("progress",progress);

	// parse command args
	char **argv;
	int argc = parse_command(command, argv);
	cmdarg_load(argc,argv);

	// write result
	return XC_SUCCESS;
}

static int server_sockfd = 0;
static void daemon_cleanup_run()
{
	if ( server_sockfd > 0 )
	{
		daemon_log("shutting down socket %d",server_sockfd);
		shutdown(server_sockfd,SHUT_RDWR);
		close(server_sockfd);
	}
}

static void daemon_process(void)
{
	int pid;
	daemon_pid = getpid();

	// *** daemon_log may not be used before this point ***
	daemon_log("***");
	daemon_log("*** new daemon starting");
	daemon_log("***");
	daemon_log("gridlabd version %d.%d.%d-%d (%s)",global_version_major,global_version_minor,global_version_patch,global_version_build,global_version_branch);
	daemon_log("workdir is '%s'", workdir);
	daemon_log("daemon pid is %d",getpid());
	atexit(daemon_cleanup);

	// save the processid
	if ( ! daemon_savepid() )
	{
		daemon_log("unable to write pidfile %s",pidfile);
		exit(XC_INIERR);
	}

	// close standard files
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	// create the socket
	int sockfd, portno=atoi(port);
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	int n;
	if ( portno <= 0 )
	{
		daemon_log("invalid port number (port=%s)",port);
		exit(XC_INIERR);
	}
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if ( sockfd < 0 ) 
	{
		daemon_log("error opening socket -- %s",strerror(errno));
		exit(XC_INIERR);
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	inet_pton(AF_INET,clientmask,&serv_addr.sin_addr);
	serv_addr.sin_port = htons(portno);
	if ( bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0 ) 
	{
		daemon_log("error binding socket %d on port %d -- %s",sockfd,portno,strerror(errno));
		exit(XC_INIERR);
	}
	int nmax = atoi(maxbacklog);
	if ( nmax <= 0 )
	{
		daemon_log("invalid maxbacklog (maxbacklog=%s)",maxbacklog);
		exit(XC_INIERR);
	}
	else if ( nmax > 128 )
	{
		daemon_log("maxbacklog is limited to 128 (maxbacklog=%s)",maxbacklog);
		nmax = 128;
	}
	if ( listen(sockfd,nmax) != 0 )
	{
		daemon_log("error listening to socket %d -- %s",sockfd,strerror(errno));
		exit(XC_INIERR);
	}
	clilen = sizeof(cli_addr);

	// install signal handlers
	signal(SIGHUP,daemon_signal);
	signal(SIGINT,daemon_signal);
	signal(SIGKILL,daemon_signal);
	signal(SIGTERM,daemon_signal);

	// start daemon loop
	while ( true ) 
	{
		server_sockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if ( server_sockfd < 0 ) 
		{
			daemon_log("accept failed on socket %d -- %s",strerror(errno));
			continue;
		}  

		// fork new process
		pid = fork();
		if ( pid < 0 )
		{
			daemon_log("forked failed -- %s",strerror(errno));
			continue;
		}
		else if ( pid > 0 ) // parent
		{
			daemon_log("forked pid %d",pid);
			continue;
		}
		else // child
		{
			atexit(daemon_cleanup_run);
			int code = daemon_run(server_sockfd);
			if ( code != XC_SUCCESS )
				exit(code);
		}
	}
	// this function only returns when a remote request is received and the command has been loaded
	return;
}

static void daemon_loadconfig(void)
{
	// access the file
	FILE *fp = fopen(global_daemon_configfile,"rt");
	if ( fp == NULL )
	{
		output_error("daemon_loadconfig(): '%s' open failed: %s",(const char*)global_daemon_configfile,strerror(errno));
		exit(XC_INIERR);
	}
	output_debug("daemon_loadconfig(): loading '%s'",(const char*)global_daemon_configfile);

	// parse the config file
	bool old_repeat = global_suppress_repeat_messages;
	global_suppress_repeat_messages = false;
	while ( ! feof(fp) )
	{
		char line[1024];
		if ( fgets(line,sizeof(line)-1,fp) != NULL )
		{
			if ( line[0] == '#' )
				continue;

			char name[1024], value[1024];
			if ( sscanf(line,"%1023[^=]=%1023[^\n]",name,value) == 2)
			{
				struct s_config *item;
				int found = 0;
				for ( item = config ; item->name != NULL ; item++ )
				{
					if ( strcmp(item->name,name)==0 )
					{
						output_debug("daemon_loadconfig(): [%s] '%s' <- '%s'", (const char*)global_daemon_configfile, name, value);
						strcpy(item->value,value);
						found = 1;
						break;
					}
				}
				if ( ! found ) 
				{
					output_error("daemon_loadconfig(): [%s] '%s' not recognized", (const char*)global_daemon_configfile, name);
					exit(XC_INIERR);
				}
			}
		}
		else if ( ferror(fp) )
		{
			output_error("%s: %s", (const char*)global_daemon_configfile, strerror(errno));
			exit(XC_INIERR);
		}
	}
	if ( feof(fp) )
	{
		output_debug("daemon_loadconfig(): end-of-file");
	}
	output_debug("daemon_loadconfig(): load of '%s' completed",(const char*)global_daemon_configfile);
	fclose(fp);
	global_suppress_repeat_messages = old_repeat;
}

static int daemon_arguments(int argc, char *argv[])
{
	int nargs = 0, portno = atoi(port);

	// process args
	while ( argc > 1 )
	{
		if ( strcmp(argv[1],"-f")==0 )
		{
			if ( argc > 2 )
			{
				strcpy((char*)global_daemon_configfile,argv[2]);
				output_debug("daemon_start(): configuration file '%s selected",(const char*)global_daemon_configfile);
				argc += 2;
				argc -= 2;
				nargs++;
			}
			else
			{
				output_error("daemon_start(): '-f' option missing configuration file name");
				exit(XC_PRCERR);
			}
		}
		else if ( strcmp(argv[1],"-p")==0 )
		{
			if ( argc > 2 )
			{
				portno = atoi(argv[2]);
				if ( portno > 0 )
				{
					output_debug("daemon_start(): port number %d selected",portno);
				}
				else
				{
					output_error("daemon_start(): port number '%s' is not valid",argv[2]);
					exit(XC_PRCERR);
				}
				argc += 2;
				argc -= 2;
				nargs++;
			}
			else
			{
				output_error("daemon_start(): '-p' option is missing port number");
				exit(XC_PRCERR);
			}
		}
		else if ( argc > 1 )
		{
			output_error("daemon_start(): argument '%s' is not recognized",argv[0],argv[1],argv[2]);			
				exit(XC_PRCERR);
		}
	}
	return nargs;
}

static int daemon_configure()
{
	pid_t pid, sid;

	// change file mode
	umask(0);

	// change the working folder
	if ( chdir(workdir) < 0 )
	{
		output_error("unable to change to workdir '%s' -- %s", workdir, strerror(errno));
		exit(XC_INIERR);
	}

	// get session id
	sid = setsid();
	if ( sid < 0 )
	{
		daemon_log("unable to set session id -- %s",strerror(errno));
		exit(XC_INIERR);
	}
	daemon_log("session id %d",sid);

	// set the user/group id
	// TODO: process setuid
	if ( geteuid() == 0 )
	{
		daemon_log("running as root is forbidden");
		exit(XC_INIERR);
	}
}

int daemon_start(int argc, char *argv[])
{
	int nargs = 0;
	if ( argc > 0 )
	{
		// process command arguments
		nargs = daemon_arguments(argc,argv);

		// access config file
		daemon_loadconfig();

		// configure as specified
		daemon_configure();
	}

	// check for existing pid
	pid_t pid = daemon_readpid();
	if ( pid != 0 )
	{
		// check to see if pid actually exists
		if ( kill(pid,0) == 0 )
		{	output_error("a daemon is already running (pid=%d)", pid);
			exit(XC_INIERR);
		}
		else
			output_warning("daemon pid %d died without cleanup its pidfile", pid);
	}	

	// fork the daemon process
	pid = fork();
	if ( pid < 0 )
	{
		output_error("daemon_start(): fork failed -- %s",argv[2],strerror(errno));
		exit(XC_PRCERR);
	}
	else if ( pid > 0 )
	{
		output_verbose("daemon_start(): process started (pid = %d)", pid);
	}
	else
	{
		daemon_process();
		return nargs;
	}		
	if ( argc > 0 ) 
		exit(0);
	else
		return nargs;
}

int daemon_stop(int argc, char *argv[])
{
	int nargs = 0;
	if ( argc > 0 )
	{
		// process command arguments
		nargs = daemon_arguments(argc,argv);

		// access config file
		daemon_loadconfig();

		// configure as specified
		daemon_configure();
	}


	// kill existing daemon
	int pid = daemon_readpid();
	if ( pid > 0 )
	{
		output_verbose("daemon_stop(): sending SIGTERM to pid %d", pid);
		int delay = 1;
		while ( kill(pid,0) == 0 && delay < 32)
		{
			kill(pid,SIGTERM);
			sleep(delay);
			delay *= 2;
		}
		if ( kill(pid,0) == 0 )
		{
			output_error("unable to kill daemon pid %d", pid);
		}
	}
	else
		output_verbose("daemon_stop(): no daemon running");
	if ( argc > 0 )
		exit(0);
	else
		return nargs;
}

int daemon_restart(int argc, char *argv[])
{
	int nargs = 0;
	if ( argc > 0 )
	{
		// process command arguments
		nargs = daemon_arguments(argc,argv);

		// access config file
		daemon_loadconfig();

		// configure as specified
		daemon_configure();
	}

	daemon_stop(0,NULL);
	return daemon_start(0,NULL);
}

int daemon_status(int argc, char *argv[])
{
	int nargs = 0;
	if ( argc > 0 )
	{
		// process command arguments
		nargs = daemon_arguments(argc,argv);

		// access config file
		daemon_loadconfig();

		// configure as specified
		daemon_configure();
	}

	// kill existing daemon
	int pid = daemon_readpid();
	if ( pid > 0 )
		output_message("gridlabd daemon is up (pid=%d)", pid);
	else
		output_message("gridlabd daemon is down");
	exit(0);
}

static void daemon_remote_kill(int sig)
{
	if ( sig == SIGALRM )
	{
		output_error("remote timeout");
		exit(XC_SIGNAL|SIGALRM);
	}
}

// this is the only code that does not run on the daemon/server side
int daemon_remote_client(int argc, char *argv[])
{
	char hostname[1024];
	int sockfd, portno = 6266, n;

	struct sockaddr_in serv_addr;
	struct hostent *server;

	char buffer[1024];
	if (argc < 2) 
	{
		output_error("remote hostname[:port] required");
		exit(XC_INIERR);
	}
	if ( sscanf(argv[1],"%[^:]:%d",hostname,&portno) < 1 )
	{
		output_error("remote hostname[:port] not recognized", portno);
		exit(XC_INIERR);
	}
	else
	{
		if ( portno < 0 )
		{
			output_error("remote port number must be positive (port=%d)", portno);
			exit(XC_INIERR);
		}
	}
	argc--;
	argv++;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
	{
		output_error("remote socket failed -- %s", strerror(errno));
		exit(XC_INIERR);
	}
	server = gethostbyname(hostname);
	if (server == NULL) 
	{
		output_error("remote hostname '%s' lookup failed -- %s", hostname, strerror(errno));
		close(sockfd);
		exit(XC_INIERR);
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	output_verbose("connecting to %s:%d",hostname,portno);
	if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
	{
		output_error("remote connection to %s:%d failed -- %s", hostname, portno, strerror(errno));
		close(sockfd);
		exit(XC_INIERR);
	}

	FILE *out = fdopen(sockfd,"w");
	FILE *in = fdopen(dup(sockfd),"r");
	bzero(buffer,sizeof(buffer));
	while ( --argc > 0 )
	{
		strcat(buffer,*++argv);
		strcat(buffer," ");
	}
	output_verbose("remote sending command [%s]",buffer);
	n = fprintf(out,"%s\n",buffer);
	fflush(out);
	if (n < 0) 
	{
		output_error("remote write failed -- %s", strerror(errno));
		exit(XC_INIERR);
	}
	bzero(buffer,sizeof(buffer));

	signal(SIGALRM,daemon_remote_kill);
	int tout = max(0,atoi(timeout));
	while ( !feof(in) )
	{
		output_verbose("remote waiting for response");
		alarm(tout);
		if ( fgets(buffer,sizeof(buffer)-1,in) )
		{
			if ( strcmp(buffer,".")!=0 )
				printf("%s\n",buffer); 	
		}
		else if ( ferror(in) ) 
		{
			output_error("remote read failed -- %s", strerror(errno));
			break;
		}
	}
	shutdown(sockfd,SHUT_RDWR);
	close(sockfd);
	exit(XC_INIERR);
}
