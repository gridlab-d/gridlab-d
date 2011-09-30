/* instance.h

 This property describes and manages a connection to another instance of 
 GridLAB-D

 */

#ifndef _INSTANCE_H
#define _INSTANCE_H

#include "property.h"
#include "timestamp.h"

typedef struct s_instance {

	/* connected host information */
	char hostname[256];		/**< hostname for instance */
	union {
		struct {
			unsigned char a,b,c,d;
		} caddr;
		long laddr;
	} addr;					/**< ipaddr for instance */
	unsigned short port;	/**< tcpport for instance */

	/* connected process information */
	char command[1024];		/**< command line sent */
	unsigned short pid;		/**< remote pid */
	
	/* cache information */
	void *cache;			/**< instance data cache */
	size_t size;			/**< cache size */
	unsigned int state;		/**< cache state */

	/* connection information */
	enum {
		CI_MMAP, /* memory map (windows localhost) */
		CI_SHMEM, /* shared memory (linux localhost) */
		CI_SOCKET, /* socket (remote host) */
	} cnxtype;
	union {
		struct {
			int hMap;
		} mmap;
		struct {
			int fd;
			int shmkey;
			int shmid;
		} shmem;
		struct {
			int sockfd;
		} socket;
	} cnxinfo

	struct s_instance *next; 
} instance;

int convert_to_instance(char *string, void *data, PROPERTY *prop);
int convert_from_instance(char *string,int size,void *data, PROPERTY *prop);
int instance_create(instance *inst);
int instance_init(instance *inst);
int instance_initall(void);
TIMESTAMP instance_sync(instance *inst, TIMESTAMP t1);
TIMESTAMP instance_syncall(TIMESTAMP t1);

#endif
