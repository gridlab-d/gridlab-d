/* $Id: collector.h 4738 2014-07-03 00:55:39Z dchassin $
 * DP Chassin - July 2012
 * Copyright (C) 2012 Battelle Memorial Institute
 */

#ifndef _COLLECTOR_H
#define _COLLECTOR_H

class collector : public gld_object {
public:
	GL_STRING(char1024,property);
	GL_STRING(char1024,group);
	GL_STRING(char1024,table);
	GL_STRING(char32,mode);
	GL_ATOMIC(int32,limit);
	GL_ATOMIC(double,interval);
	GL_ATOMIC(object,connection);
	GL_ATOMIC(set,options);
private:
	MYSQL *mysql;
	database *db;
	size_t n_aggregates; /// number of aggregates found
	gld_aggregate *list; ///< list of aggregates
	char **names; ///< list of aggregate names
public:
	TIMESTAMP next_t;
public:
	collector(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP commit(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static collector *defaults;
};

#endif
