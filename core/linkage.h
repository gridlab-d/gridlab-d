/* @file linkage.h 
 */

#ifndef _LINKAGE_H
#define _LINKAGE_H

#include "timestamp.h"
#include "property.h"
#include "object.h"
typedef struct s_instance instance;

typedef enum {
	LT_UNKNOWN = 0,			 ///<
	LT_MASTERTOSLAVE = 1,	 ///<
	LT_SLAVETOMASTER = 2,	 ///<
} LINKAGETYPE;
typedef struct s_linkage {
	LINKAGETYPE type; ///<
	struct {
		OBJECT *obj; ///<
		PROPERTY *prop; ///<
	} target;
	struct {
		char obj[64]; ///<
		PROPERTYNAME prop; ///<
	} local,  ///<
	  remote; ///<
	char *addr;  ///< buffer addr in MESSAGE
	size_t size;	 ///< buffer size in MESSAGE
	struct s_linkage *next; ///<
} linkage; ///<

int linkage_create_reader(instance *inst, char *fromobj, char *fromvar, char *toobj, char *tovar);
int linkage_create_writer(instance *inst, char *fromobj, char *fromvar, char *toobj, char *tovar);
int linkage_init(instance *inst, linkage *lnk);
void linkage_master_to_slave(linkage *lnk);
void linkage_slave_to_master(linkage *lnk);

#endif
