/* @file linkage.h 
 */

#ifndef _LINKAGE_H
#define _LINKAGE_H

#include "timestamp.h"
#include "property.h"
#include "object.h"

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
	size_t name_size;
	size_t prop_size;
	struct s_linkage *next; ///<
} linkage; ///<

#endif
