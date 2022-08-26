/// $Id$
/// @file xml.h
/// @addtogroup connection
/// @{

#ifndef _XML_H
#define _XML_H

#include "gridlabd.h"
#include "native.h"

typedef enum {
	UTF8=0, ///< specifies 8-bit XML encoding
	UTF16=1, ///< specifies 16-bit XML encoding
} XML_UTFENCODING; /// UTF encoding specification

class xml : public native {
public:
	GL_ATOMIC(enumeration,encoding);
	GL_STRING(char8,version);
	GL_STRING(char1024,schema);
	GL_STRING(char1024,stylesheet); 
	// TODO add published properties here

private:
	// TODO add other properties here

public:
	// required implementations
	xml(MODULE*);
	int create(void);
	int init(OBJECT*);
	int precommit(TIMESTAMP);
	TIMESTAMP presync(TIMESTAMP);
	TIMESTAMP sync(TIMESTAMP);
	TIMESTAMP postsync(TIMESTAMP);
	TIMESTAMP commit(TIMESTAMP,TIMESTAMP);
	int prenotify(PROPERTY*,char*);
	int postnotify(PROPERTY*,char*);
	int finalize();
	TIMESTAMP plc(TIMESTAMP);
	int link(char *value);
	int option(char *value);
	void term(TIMESTAMP);
	// TODO add other event handlers here

public:
	// special variables for GridLAB-D classes
	static CLASS *oclass;
	static xml *defaults;
};

#endif /// @} _XML_H 

