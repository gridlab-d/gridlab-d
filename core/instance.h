/* @file instance.h

 This property describes and manages a connection to another instance of 
 GridLAB-D

 */

#ifndef _INSTANCE_H
#define _INSTANCE_H

#include "property.h"
#include "timestamp.h"
#include "pthread.h"
#include "linkage.h"

typedef struct s_message {
	size_t usize;		///< message size (buffer memory usage in bytes)
	size_t asize;		///< message size (buffer memory allocation in bytes)
	unsigned int id;	///< slave instance id 
	TIMESTAMP ts;		///< timestamp
	char data;			///< first character in link data
} MESSAGE; ///< message cache structure

typedef struct s_instance {

	unsigned int id;
	char model[1024];
	pthread_t threadid;

	/* linkage information */
	linkage *read;	///< link read from slave
	linkage *write; ///< links written to slave

	/* connected host information */
	char hostname[256];		///< hostname for instance
	union {
		struct {
			unsigned char a,b,c,d;
		} caddr;
		long laddr;
	} addr;					///< ipaddr for instance
	unsigned short port;	///< tcpport for instance

	/* connected process information */
	char command[1024];		///< command line sent
	unsigned short pid;		///< remote pid
	
	/* cache information */
	unsigned int64 cacheid;	///< cache id
	MESSAGE *cache;			///< instance data cache
	size_t cachesize;		///< cache size

	/* connection information */
	enum {
		CI_MMAP, ///< memory map (windows localhost)
		CI_SHMEM, ///< shared memory (linux localhost)
		CI_SOCKET, ///< socket (remote host)
	} cnxtype;
	union {
#ifdef WIN32 // windows
		struct {
			void* hMap; ///<
			void* hMaster; ///<
			void* hSlave; ///<
		};
#else // linux/unix
		struct {
			int fd; ///<
			int shmkey; ///<
			int shmid; ///<
		};
#endif
		struct {
			int sockfd; ///<
		};
	};
	struct s_instance *next;  ///<
} instance; ///<

instance *instance_create(char *name);
int instance_init(instance *inst);
int instance_initall(void);
int instance_slave_init(void);
int instance_slave_wait(void);
void instance_slave_done(void);
TIMESTAMP instance_presync(instance *inst, TIMESTAMP t1);
TIMESTAMP instance_sync(instance *inst, TIMESTAMP t1);
TIMESTAMP instance_postsync(instance *inst, TIMESTAMP t1);
TIMESTAMP instance_syncall(TIMESTAMP t1);
int instance_add_linkage(instance *, linkage *);

#endif
