/* $Id$
 *
 */

#ifndef _STREAM_H
#define _STREAM_H

#include <stdio.h>
#include "platform.h"
#include "class.h"
#include "object.h"

/* stream flags */
#define SF_IN		0x0001
#define SF_OUT		0x0002
#define SF_STR		0x0004

typedef size_t (*STREAMCALL)(FILE *fp,unsigned int flags);
typedef const char *TOKEN;
typedef unsigned int uint;
typedef unsigned char uchar;

#ifdef __cplusplus
extern "C" {
#endif

void stream_register(STREAMCALL);
size_t stream(FILE *fp, int flags);
char* stream_context();

#define stream_type(T) size_t stream_##T(void*,size_t,PROPERTY*p)
#include "stream_type.h"
#undef stream_type

#ifdef __cplusplus
}

#endif


#endif
