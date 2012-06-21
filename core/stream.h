/* $Id$
 *
 */

#ifndef _STREAM_H
#define _STREAM_H

#include <stdio.h>
#include "platform.h"
#include "class.h"
#include "object.h"

/* block flags */
#define SF_MODULES 0x0100
#define SF_GLOBALS 0x0200
#define SF_CLASSES 0x0300
#define SF_OBJECTS 0x0400
#define SF_ALL     0x0f00

typedef enum {STREAM_NONE=0, STREAM_READ=1, STREAM_WRITE=2} STREAMDIR;

#ifdef __cplusplus
class Stream {
private:
	const FILE *fp;
	const int flags;
	STREAMDIR dir;
public:
	Stream(FILE *fp, int flags);
	~Stream(void);
public:
	bool write(); ///< write the current model state to the stream
	bool read();  ///< read the current model state from the stream
	void write(void *buffer, int len);
	void write(char *string);
	void read(void *buffer, int len);
	void read(char *string);
	const char *error(); ///< get the last error reported
private:
	void exception(char *fmt, ...); ///< stream exception generator
	void sync(); ///< stream synchronization functions
	void sync_objects();
};

class Object {
private:
	const OBJECT *obj;
public:
	inline Object(OBJECT *p) : obj(p) {};
public:
	void sync(Stream *s);
};

extern "C" {
#endif
int64 stream_out(FILE *fp, int flags);
int64 stream_in(FILE *fp, int flags);
char* stream_context();
int stream_in_double(FILE*,void*,PROPERTY*);
int stream_out_double(FILE*,void*,PROPERTY*);
#ifdef __cplusplus
}
#endif


#endif
