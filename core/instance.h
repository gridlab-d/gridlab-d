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
#include "lock.h"

#define HS_SYN		"GLDMTR"
#define HS_ACK		"GLDSND"
// note trailing space for CBK
#define HS_CBK		"GLDSLV "
#define HS_RSP		"GLDRDY"
// note trailing space for CMD
#define HS_CMD		"GLDCMD "
#define HS_FAIL		"GLDFAIL"
#define MSG_GLM		"GLDGLM"
#define MSG_LINK	"GLDLNKS"
#define MSG_INST	"GLDINST"
#define MSG_DATA	"GLDDATA"
#define MSG_OK		"GLDOKAY"
#define MSG_START	"GLDSTART"
#define MSG_ERR		"GLDERROR"
#define MSG_DONE	"GLDDONE"

typedef enum {
	IST_DEFAULT = 0,
	IST_MASTER_INIT = 1,
	IST_MASTER_RUN = 2,
	IST_MASTER_WAIT = 3,
	IST_MASTER_DONE = 4,
	IST_SLAVE_INIT = 5,
	IST_SLAVE_RUN = 6,
	IST_SLAVE_WAIT = 7,
	IST_SLAVE_DONE = 8,
} INSTANCE_STATE;

typedef enum {
	CI_MMAP, ///< memory map (windows localhost)
	CI_SHMEM, ///< shared memory (linux localhost)
	CI_SOCKET, ///< socket (remote host)
} CNXTYPE;

typedef struct s_message {
	size_t usize;				///< message size (buffer memory usage in bytes)
	size_t asize;				///< message size (buffer memory allocation in bytes)
	int32 id;					///< slave instance id 
	int32 hard_event;			///< number of observed hard events
	TIMESTAMP ts;				///< timestamp
	int16 name_size;
	int16 data_size;
	char data;					///< first character in link data
} MESSAGE; ///< message cache structure

typedef struct s_message_wrapper {
	MESSAGE *msg;
	int16 *name_size;
	int16 *data_size;
	char *name_buffer;
	char *data_buffer;
} MESSAGEWRAPPER;

typedef struct s_instance {

	// master info
	unsigned int id;
	char model[1024];
	char execdir[1024];
	pthread_t threadid;

	/* linkage information */
	linkage *read;	///< link read from slave
	linkage *write; ///< links written to slave

	// this block can pull double duty for connecting back up. -mh
	/* connected host information */
	char hostname[256];		///< hostname for instance
	union {
		struct {
			unsigned char a,b,c,d;
		} caddr;
		long laddr;
	} addr;					///< ipaddr for instance
	unsigned short port;	///< tcpport for instance slavenode
	unsigned short return_port;	///< tcpport for instance to connect back to

	// master info
	/* connected process information */
	char command[1024];		///< command line sent
	unsigned short pid;		///< remote pid OR local slaveproc pid
	
	// both sides
	/* cache information */
	unsigned int64 cacheid;	///< cache id
	MESSAGEWRAPPER *message;
	MESSAGE *cache;			///< instance data cache
	char *buffer;
	char *filemap;
	size_t cachesize;		///< cache size
	size_t prop_size;
	size_t name_size;
	size_t buffer_size;
	int16 writer_count;
	int16 reader_count;

	/* connection information */
	char32 cnxtypestr;
	CNXTYPE cnxtype;
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
			FILE *stream;
			pthread_mutex_t sock_lock;
			pthread_cond_t sock_signal;
			pthread_mutex_t wait_lock;
			pthread_cond_t wait_signal;
			int has_data;
			int has_data_lock;
		};
	};
	struct s_instance *next;  ///<
} instance; ///<

typedef struct s_instance_pickle {
	unsigned int64	cacheid;
	int16	cachesize;
	int16	name_size;
	int16	prop_size;
	int16	id;
	TIMESTAMP		ts;
} INSTANCE_PICKLE;

STATUS messagewrapper_init(MESSAGEWRAPPER **msgwpr,	MESSAGE *msg);
instance *instance_create(char *name);
STATUS instance_init(instance *inst);
STATUS instance_initall(void);
void instance_master_done(TIMESTAMP t1);
STATUS instance_slave_init(void);
int instance_slave_wait(void);
void instance_slave_done(void);
TIMESTAMP instance_presync(instance *inst, TIMESTAMP t1);
TIMESTAMP instance_sync(instance *inst, TIMESTAMP t1);
TIMESTAMP instance_postsync(instance *inst, TIMESTAMP t1);
TIMESTAMP instance_syncall(TIMESTAMP t1);
int instance_add_linkage(instance *, linkage *);
STATUS instance_dispose();

int linkage_create_reader(instance *inst, char *fromobj, char *fromvar, char *toobj, char *tovar);
int linkage_create_writer(instance *inst, char *fromobj, char *fromvar, char *toobj, char *tovar);
STATUS linkage_init(instance *inst, linkage *lnk);
STATUS linkage_master_to_slave(char *buffer, linkage *lnk);
STATUS linkage_slave_to_master(char *buffer, linkage *lnk);

void printcontent(unsigned char *data, size_t len);

#endif
