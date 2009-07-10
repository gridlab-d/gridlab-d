
#ifndef _ENDUSE_H
#define _ENDUSE_H

#include "class.h"
#include "timestamp.h"

typedef struct s_enduse {
	struct s_enduse *next;
} enduse;

TIMESTAMP enduse_syncall(TIMESTAMP t1);
int convert_to_enduse(char *string, void *data, PROPERTY *prop);
int convert_from_enduse(char *string,int size,void *data, PROPERTY *prop);

#endif