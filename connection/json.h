/** $Id$

 Object class header

 **/

#ifndef _JSON_H
#define _JSON_H

#include "gridlabd.h"
#include "native.h"

size_t convert_to_hex(char *hex, size_t hexlen, void *buffer, size_t buflen);
size_t convert_from_hex(void *buffer, size_t buflen, char *hex, size_t hexlen);

typedef enum {
	JT_VOID,
	JT_LIST,
	JT_REAL,
	JT_INTEGER,
	JT_STRING,
} JSONTYPE;
typedef struct _jsonlist {
	JSONTYPE type;
	char tag[32];
	union {
		double real;
		int64 integer;
		struct _jsonlist *list;
	};
	char string[1024];
	struct _jsonlist *parent; // parent list (NULL for head)
	struct _jsonlist *next;
} JSONLIST;

class json : public native {
public:
	GL_ATOMIC(double,version);
	// TODO add published properties here

private:
	// TODO add other properties here

public:
	// required implementations
	json(MODULE*);
	int create(void);
	int init(OBJECT*);
	int precommit(TIMESTAMP);
	TIMESTAMP presync(TIMESTAMP);
	TIMESTAMP sync(TIMESTAMP);
	TIMESTAMP postsync(TIMESTAMP);
	TIMESTAMP commit(TIMESTAMP,TIMESTAMP);
	int prenotify(PROPERTY*,char*);
	int postnotify(PROPERTY*,char*);
	int finalize(void);
	TIMESTAMP plc(TIMESTAMP);
	int link(char *value);
	int option(char *value);
	void term(TIMESTAMP);
	// TODO add other event handlers here

public:
	static JSONLIST *parse(char *buffer);
	static JSONLIST *find(JSONLIST *list, const char *tag);
	static char *get(JSONLIST *list, const char *tag);
	static void destroy(JSONLIST *list);

public:
	// special variables for GridLAB-D classes
	static CLASS *oclass;
	static json *defaults;
};

#endif // _JSON_H
