/* $Id: player.h 4738 2014-07-03 00:55:39Z dchassin $
 * DP Chassin - July 2012
 * Copyright (C) 2012 Battelle Memorial Institute
 */

#ifndef _PLAYER_H
#define _PLAYER_H

class player : public gld_object {
public:
	GL_STRING(char256,property);
	GL_STRING(char1024,table);
	GL_STRING(char32,mode);
	GL_STRING(char8,filetype);
	GL_ATOMIC(object,connection);
	GL_ATOMIC(set,options);
private:
	gld_property target;
	gld_unit unit;
	char256 field;
	double scale;
	database *db;
	MYSQL *mysql;
	MYSQL_RES *data;
	MYSQL_ROW row;
	unsigned long n_rows;
	MYSQL_FIELD *fields;
	unsigned long n_fields;
	TIMESTAMP next_t;
	unsigned long row_num;
public:
	player(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	int precommit(TIMESTAMP t0);
	TIMESTAMP commit(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static player *defaults;
};

#endif
