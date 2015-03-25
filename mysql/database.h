/** $Id: database.h 4738 2014-07-03 00:55:39Z dchassin $

 General purpose assert objects

 **/

#ifndef _DATABASE_H
#define _DATABASE_H

#include "gridlabd.h"

#ifdef WIN32
#undef int64
#include <winsock2.h>
#define int64 _int64
#else
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/errno.h>
#define SOCKET int
#define INVALID_SOCKET (-1)
#endif

#include <mysql.h>

#ifdef DLMAIN
#define EXTERN
#define INIT(A) = A
#else
#define EXTERN extern
#define INIT(A)
#endif

EXTERN char default_hostname[256] INIT("127.0.0.1");
EXTERN char default_username[32] INIT("gridlabd");
EXTERN char default_password[32] INIT("");
EXTERN char default_schema[256] INIT("gridlabd");
EXTERN int32 default_port INIT(3306);
EXTERN char default_socketname[1024] INIT("/tmp/mysql.sock");
EXTERN int64 default_clientflags INIT(CLIENT_LOCAL_FILES);
EXTERN MYSQL *mysql_client INIT(NULL);

#define DBO_SHOWQUERY 0x0001 ///< show SQL query when verbose is on
#define DBO_NOCREATE 0x0002 ///< prevent automatic creation of schema
#define DBO_DROPSCHEMA 0x0004 ///< drop schema before using it
#define DBO_OVERWRITE 0x0008	///< overwrite existing file when dumping and backing up

class database : public gld_object {
public:
	GL_STRING(char256,hostname);
	GL_STRING(char32,username);
	GL_STRING(char32,password);
	GL_STRING(char256,schema);
	GL_ATOMIC(int32,port);
	GL_STRING(char1024,socketname);
	GL_ATOMIC(int64,clientflags);
	GL_ATOMIC(set,options);
	GL_STRING(char1024,on_init);
	GL_STRING(char1024,on_sync);
	GL_STRING(char1024,on_term);
	GL_ATOMIC(double,sync_interval);

	// mysql handle
private:
	MYSQL *mysql;
public:
	inline MYSQL *get_handle() { return mysql; };

	// term list
private:
	database *next;
	static database *first;
	static database *last;
public:
	static inline database *get_first(void) { return first; };
	inline database *get_next(void) { return next; }

public:
	/* required implementations */
	database(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP commit(TIMESTAMP from, TIMESTAMP to);
	void term(void);

public:
	static CLASS *oclass;
	static database *defaults;

public:
	// special functions
	const char *get_last_error(void);
	bool table_exists(char *table);
	bool query(char *query,...);
	unsigned int64 get_last_index(void);
	MYSQL_RES *select(char *query,...);
	MYSQL_RES get_next(MYSQL_RES*res);
	int run_script(char *file);
	size_t backup(char *file);
#define TD_APPEND 0x0001 ///< table dump is appended to file
#define TD_BACKUP 0x0002 ///< table dump is formatted as SQL backup dump
	size_t dump(char *table, char *file=NULL, unsigned long options=0x0000);

	char *get_sqltype(gld_property &p);
	char *get_sqldata(char *buffer, size_t size, gld_property &p, double scale=1.0);
	bool get_sqlbind(MYSQL_BIND &value,gld_property &target, my_bool *error=NULL);
};

EXTERN database *last_database INIT(NULL);

#include "recorder.h"
#include "player.h"
#include "collector.h"

#endif // _ASSERT_H
