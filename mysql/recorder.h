/* $Id: recorder.h 4738 2014-07-03 00:55:39Z dchassin $
 * DP Chassin - July 2012
 * Copyright (C) 2012 Battelle Memorial Institute
 */

#ifndef _RECORDER_H
#define _RECORDER_H

#define MO_DROPTABLES	0x0001	// drop tables option flag
#define MO_USEUNITS		0x0002	// add units to column names
#define MO_NOCREATE		0x0004	// do not automatically create tables

class recorder : public gld_object {
public:
	GL_STRING(char1024,property);
	GL_STRING(char32,trigger);
	GL_STRING(char1024,table);
	GL_STRING(char32,mode);
	GL_ATOMIC(int32,limit);
	GL_ATOMIC(double,interval);
	GL_ATOMIC(object,connection);
	GL_ATOMIC(set,options);
private:
	bool enabled;
	gld_property target;
	gld_unit unit;
	char256 field;
	double scale;
	database *db;
	MYSQL_STMT *insert;
	MYSQL_BIND value;
	double real;
	int64 integer;
	char1024 string;
	my_bool stmt_error;
	bool trigger_on;
	char compare_op[16];
	char compare_val[32];
public:
	inline bool get_trigger_on(void) { return trigger_on; };
	inline bool get_enabled(void) { return enabled; };
public:
	recorder(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP commit(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static recorder *defaults;
};

#endif
