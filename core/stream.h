/* $Id$
 *
 */

#ifndef _STREAM_H
#define _STREAM_H

#include <stdio.h>
#include "platform.h"
#include "class.h"

/* block flags */
#define SF_MODULES 0x0100
#define SF_GLOBALS 0x0200
#define SF_CLASSES 0x0300
#define SF_OBJECTS 0x0400
#define SF_ALL     0x0f00

int64 stream_out(FILE *fp, int flags);
int64 stream_in(FILE *fp, int flags);
char* stream_context();

int stream_in_double(FILE*,void*,PROPERTY*);
int stream_out_double(FILE*,void*,PROPERTY*);

#endif
