/* $Id$
 * DP Chassin - July 2012
 * Copyright (C) 2012 Battelle Memorial Institute
 */

#ifndef _COLLECTOR_H
#define _COLLECTOR_H

class collector : public gld_object {
public:
	GL_STRING(char1024,property);
	GL_STRING(char32,trigger);
	GL_STRING(char32,table);
	GL_STRING(char32,mode);
	GL_ATOMIC(int32,limit);
	GL_ATOMIC(double,interval);
	GL_ATOMIC(object,connection);
private:
	MYSQL *mysql;
public:
	collector(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
public:
	static CLASS *oclass;
	static collector *defaults;
};

#endif
