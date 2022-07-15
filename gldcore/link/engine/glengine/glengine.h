// glengine.h

#ifndef _GLENGINE_H
#define _GLENGINE_H

#include <stdarg.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <vector>
#include <sstream>
#include <iostream>
#include <cassert>

#define TS_NEVER ((int64)(((unsigned int64)-1)>>1))

#include "absconnection.h"

#ifdef _WIN32
	#ifdef int64
	#undef int64 // wtypes.h uses the term int64
	#endif
	#define socklen_t int
	#define snprintf _snprintf
	#ifndef int64
	#define int64 _int64
	#endif
	#define int64_t _int64
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <sys/errno.h>
	#include <sys/wait.h>
	#include <netdb.h>
	#include <unistd.h>
	#define INVALID_SOCKET (-1)
	#define SOCKET_ERROR (-1)
	#define int64 long long
	#define SOCKET int
	#define vsprintf_s vsnprintf
	#define sprintf_s snprintf
#endif


#include <map>

#include "glproperty.h"
#include "absconnection.h"


class glengine {

	/// Debug level
	/// 0: no debugging messages
	/// 1: thread control messages
	/// 2: send/recv messages
private:
	int debug_level;
public:
	inline void set_debug_level(int n=0) {debug_level=n; };
	inline int get_debug_level(void) { return debug_level; };

	/// command string
private:
#ifdef _WIN32
	PROCESS_INFORMATION gldpid;
#else
	pid_t gldpid;
#endif
	void startGLD(const char *gldexecpath
		,const char *args[]);
	/// options
private:
	typedef enum {GEO_SYNC=0x00, GEO_ASYNC=0x01} GLENGINEOPTION;
	GLENGINEOPTION options;

	/// thread id
private:
	int major, minor, patch, build;
	char branch[32];
	char protocol[32];
	int cachesize,timeout;
	char buffer[1500];
	void protocolConnect();
public:

	/// socket data
private:
	absconnection *interface;

public:
	glengine(void);
	~glengine(void);
	/// shutdown the current gridlabd instance and disconnect
	int shutdown(const int signum=SIGTERM);

public:
	/// establish a local connection to a gridlabd instance, this will launch gridlabd
	void connect(ENGINE_SOCK_TYPE socktype,const char *pathToGLD,const char *parameters,...);
	/// establish a local connection using UDP to a gridlabd instance, this will launch gridlabd
	void connect(const char *pathToGLD,const char *parameters,...);
	
	/// establish a connectiong to a remote gridlabd instance, remote instance should be manually started!!
	void connect(ENGINE_SOCK_TYPE socktype,const string &ip,const int &port=6267);

	/// send a signal to the current gridlabd instance
	//int signal(const int signum=0);

	/// get data from the cache
	int get(const char *name, char *value, size_t len) const;
	int get(const string name,string &value) const;

	///get names of data values in cache
	vector<string> getExportNames() const;
	vector<string> getImportNames() const;
	///get the type of a value
	int getType(const string name,string &type) const; 

	/// set data in the cache
	int set(const char *name, const char *value);
	int set(const string name,const string value);

	/// perform the next sync event
	void sync_finalize(const int64_t syncto) ;
	int sync_start(int64_t &syncCurrentTime) ;
	  
	/// debug, warning, exception output messages (all to stdout)
	void debug(int level, const char *fmt, ...);
	void warning(const char *fmt, ...);
	//int error(const char *fmt, ...);
	void exception(const char *fmt, ...);
	
	/// sleep (0=reschedule)
	void sleep(unsigned int msec=0);

private:
	///cache
	map<int,glproperty*> exports_cache,imports_cache;
	map<string,pair<PROPOERTYCONTEXT,int> > cacheIndex;
	glproperty *getFromCache(string name) const;
	
	void addtocache(glproperty* given);
	
	//boolean flag for testing whether the engine has already been shutdown.
	bool shut;
	/// send a cache update
	int send(const char *fmt, ...);

	/// recv protocol messages
	bool recvTime(int64_t &syncTime);
	bool recvSyncState(string &state);
	bool recvExport(int &index,string &value);
	bool recvProtocol(char *given);
	bool recvCacheSize(int &cacheSize);
	bool recvTimeout(int &timeout);
	bool recvProperty(char *prop);
	int recv(char *buffer,const int &maxLen,bool setTimeout=true);
	bool recv_gldversion();
	///recv exports_cache
	void recv_exports();
	
	///send imports_cache
	void send_imports();
};

#endif
